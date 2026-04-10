#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/Helpers/CallValidator.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Variable.hpp>
#include <IgniLang/Semantic/Helpers/Export.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Class.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Function.hpp>
#include <IgniLang/Semantic/Helpers/Import.hpp>
#include <IgniLang/Semantic/Helpers/MemberResolver.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <algorithm>
#include <complex>

namespace igni::sem
{

class SemanticAnalyzer final : public ast::BaseAstVisitor
{
public:
	SemanticAnalyzer()
	{
		m_context.evaluateFunctionCallback = [this](const auto& node) {
			return this->Evaluate(node);
		};

		m_context.instantiateClassCallback = [this](const auto& tmpl, const auto& args) {
			return Generic::Class::Instantiate(tmpl, args, m_context);
		};

		m_context.instantiateFunctionCallback = [this](const auto& tmpl, const auto& args) {
			return Generic::Function::Instantiate(tmpl, args, m_context);
		};

		InitBuiltins();
	}

	void Analyze(const std::vector<std::unique_ptr<ast::Program>>& programs)
	{
		for (const auto& prog : programs)
		{
			m_currentProgram = prog.get();
			RegisterProgramDeclarations(prog.get());
		}

		for (const auto& prog : programs)
		{
			m_currentProgram = prog.get();
			m_context.env.PushScope();

			if (const bool isGlobal = prog->packageName.Empty() || prog->packageName == "global"; !isGlobal)
			{
				if (const Symbol* modSym = m_context.env.Resolve(prog->packageName))
				{
					if (const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type))
					{
						for (const auto& [name, type] : modType->exports)
						{
							m_context.env.Define(name, type, true);
						}
					}
				}
			}

			for (const auto& imp : prog->imports)
			{
				if (imp)
				{
					imp->Accept(*this);
				}
			}

			for (std::size_t i = 0; i < prog->statements.size(); ++i)
			{
				if (prog->statements[i])
				{
					prog->statements[i]->Accept(*this);
				}
			}

			while (!m_context.pendingStatements.empty())
			{
				auto pending = std::move(m_context.pendingStatements);
				m_context.pendingStatements.clear();

				for (auto& stmt : pending)
				{
					prog->statements.push_back(std::move(stmt));

					prog->statements.back()->Accept(*this);
				}
			}

			m_context.env.PopScope();
		}
	}

	[[nodiscard]] std::shared_ptr<ClassType> GetClassType(const re::String& className) const
	{
		if (const auto itInst = m_context.instantiatedClasses.find(className); itInst != m_context.instantiatedClasses.end())
		{
			return itInst->second;
		}

		if (const auto itAll = m_context.allClassTypes.find(className); itAll != m_context.allClassTypes.end())
		{
			return itAll->second;
		}

		return nullptr;
	}

	[[nodiscard]] std::unordered_set<re::String> GetGlobalNames() const
	{
		auto globals = m_context.env.GetGlobalNames();
		globals.insert(m_context.allFunctionNames.begin(), m_context.allFunctionNames.end());

		return globals;
	}

	[[nodiscard]] const std::unordered_map<re::String, re::String>& GetImportAliases() const
	{
		return m_context.importAliases;
	}

	[[nodiscard]] const std::unordered_set<re::String>& GetExternalFunctions() const
	{
		return m_context.externalFunctions;
	}

	// ==========================================
	// EXPRESSIONS
	// ==========================================
	void Visit(const ast::LiteralExpr* node) override
	{
		// clang-format off
		switch (node->token.type)
		{
		case TokenType::IntConst:    m_currentExprType = m_context.tInt; break;
		case TokenType::FloatConst:  m_currentExprType = m_context.tDouble; break;
		case TokenType::StringConst: m_currentExprType = m_context.tString; break;
		case TokenType::KwTrue:
		case TokenType::KwFalse:     m_currentExprType = m_context.tBool; break;
		default:                     m_currentExprType = m_context.tUnit; break;
		}
		// clang-format on
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		if (node->name == "super")
		{
			if (!m_context.location.currentClass || !m_context.location.currentClass->baseClass)
			{
				throw std::runtime_error("Semantic Error: Cannot use 'super' outside of a derived class");
			}
			m_currentExprType = m_context.location.currentClass->baseClass;

			return;
		}

		const Symbol* sym = m_context.env.Resolve(node->name);
		if (!sym)
		{
			throw std::runtime_error("Semantic Error: Undefined variable '" + node->name + "'");
		}
		m_currentExprType = sym->type;
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		const auto leftType = Evaluate(node->object.get());

		if (const auto modType = std::dynamic_pointer_cast<ModuleType>(leftType))
		{
			m_currentExprType = Export::Resolve(modType, node->member);
		}
		else if (const auto classType = std::dynamic_pointer_cast<ClassType>(leftType))
		{
			m_currentExprType = MemberResolver::ResolveAccess(classType, node->member, m_context);
		}
		else
		{
			throw std::runtime_error("Semantic Error: Cannot access member '" + node->member + "' on primitive type '" + leftType->name + "'");
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		const auto leftType = Evaluate(node->left.get());
		const auto rightType = Evaluate(node->right.get());

		std::shared_ptr<SemanticType> mathType = TypeResolver::PromoteMathTypes(leftType.get(), rightType.get(), m_context);
		if (!mathType)
		{
			mathType = leftType;
		}

		if (const auto hashed = node->op.Hashed();
			hashed == "<"_hs
			|| hashed == ">"_hs
			|| hashed == "=="_hs
			|| hashed == "!="_hs
			|| hashed == "<="_hs
			|| hashed == ">="_hs)
		{
			m_currentExprType = m_context.tBool;
		}
		else
		{
			m_currentExprType = mathType;
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		m_currentExprType = Evaluate(node->operand.get());
	}

	void Visit(const ast::IndexExpr* node) override
	{
		const auto arrType = Evaluate(node->array.get());
		const auto indexType = Evaluate(node->index.get());

		TypeResolver::ExpectAssignable(indexType.get(), m_context.tInt.get(), "array index");

		if (const auto classType = std::dynamic_pointer_cast<ClassType>(arrType))
		{ // allow index access operation to any class with get(Int) method
			if (const auto it = classType->methods.find("get"); it != classType->methods.end())
			{
				if (const auto funType = std::dynamic_pointer_cast<FunctionType>(it->second))
				{
					m_currentExprType = funType->returnType;
					return;
				}
			}
		}

		throw std::runtime_error("Semantic Error: Type '" + arrType->name + "' does not support indexing (missing 'get' method)");
	}

	void Visit(const ast::CallExpr* node) override
	{
		bool isSuperCall = false;

		const auto calleeType = Evaluate(node->callee.get());

		std::vector<std::shared_ptr<SemanticType>> argTypes;
		for (std::size_t i = 0; i < node->arguments.size(); ++i)
		{
			argTypes.emplace_back(Evaluate(node->arguments[i].get()));
		}

		std::vector<std::shared_ptr<SemanticType>> explicitTypeArgs;
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{ // Explicit type arguments ident::<T>
			isSuperCall = id->name.Hashed() == "super"_hs;
			for (const auto& t : id->typeArgs)
			{
				explicitTypeArgs.push_back(TypeResolver::Resolve(t.get(), m_context));
			}
		}
		else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			for (const auto& t : memAccess->typeArgs)
			{
				explicitTypeArgs.push_back(TypeResolver::Resolve(t.get(), m_context));
			}
		}

		std::shared_ptr<FunctionType> concreteTarget = nullptr;
		bool isMethodCall = false;

		if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(calleeType))
		{ // Generic class constructor
			if (explicitTypeArgs.empty())
			{
				const ast::ConstructorDecl* ctorAst = nullptr;
				const auto it = std::ranges::find_if(classTmpl->astNode->members, [](const auto& member) {
					return dynamic_cast<const ast::ConstructorDecl*>(member.get());
				});
				if (it != classTmpl->astNode->members.end())
				{
					ctorAst = dynamic_cast<const ast::ConstructorDecl*>(it->get());
				}

				for (const auto& member : classTmpl->astNode->members)
				{ // Finding constructor in class members
					if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						ctorAst = ctor;
						break;
					}
				}

				if (ctorAst)
				{
					explicitTypeArgs.resize(classTmpl->typeParams.size(), nullptr);

					// Here generic class is not yet instantiated, so we don't need to add 'this' as the first argument
					for (std::size_t i = 0; i < argTypes.size() && i < ctorAst->parameters.size(); ++i)
					{
						const auto paramTypeAst = ctorAst->parameters[i].type.get();
						TypeResolver::InferTypeArguments(argTypes[i], paramTypeAst, classTmpl->typeParams, explicitTypeArgs);
					}

					for (const auto& arg : explicitTypeArgs)
					{
						if (!arg)
						{
							throw std::runtime_error("Semantic Error: Could not infer type arguments for generic class '" + classTmpl->name + "'. Use explicit ::<T> syntax.");
						}
					}
				}
				else
				{
					throw std::runtime_error("Semantic Error: Generic class '" + classTmpl->name + "' has no parameters to infer types from. Use explicit ::<T> syntax.");
				}
			}
			const auto concreteClass = m_context.instantiateClassCallback(classTmpl, explicitTypeArgs);
			const auto it = concreteClass->methods.find(concreteClass->name);
			if (it == concreteClass->methods.end())
			{
				throw std::runtime_error("Semantic Error: Constructor not found for generic class '" + concreteClass->name + "'");
			}
			concreteTarget = std::dynamic_pointer_cast<FunctionType>(it->second);

			const_cast<ast::CallExpr*>(node)->isConstructorCall = true;
			isMethodCall = true;

			if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			{
				id->name = classTmpl->astNode->isExternal ? classTmpl->name : concreteClass->name;
			}
		}
		else if (const auto classType = std::dynamic_pointer_cast<ClassType>(calleeType))
		{ // Non-generic class constructor
			const auto it = classType->methods.find(classType->name);
			if (it == classType->methods.end())
			{
				throw std::runtime_error("Semantic Error: Constructor not found for class '" + classType->name + "'");
			}
			concreteTarget = std::dynamic_pointer_cast<FunctionType>(it->second);

			const_cast<ast::CallExpr*>(node)->isConstructorCall = true;
			isMethodCall = true;
		}
		else if (const auto funTmpl = std::dynamic_pointer_cast<GenericFunctionTemplate>(calleeType))
		{ // Generic function
			isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr;
			concreteTarget = CallValidator::ResolveAndInstantiateGeneric(funTmpl, explicitTypeArgs, argTypes, m_context);

			if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			{
				id->name = funTmpl->isExternal ? funTmpl->name : concreteTarget->name;
			}
		}
		else if (const auto funType = std::dynamic_pointer_cast<FunctionType>(calleeType))
		{ // Non-generic function
			isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr;
			concreteTarget = funType;
		}
		else
		{
			throw std::runtime_error("Semantic Error: Attempt to call a non-callable type");
		}

		const auto mutableNode = const_cast<ast::CallExpr*>(node);
		mutableNode->isSuperCall = isSuperCall;

		if (!concreteTarget)
		{
			throw std::runtime_error("Internal Compiler Error: concreteTarget is null for call to '" + re::String(node->callee ? "some callee" : "unknown") + "'");
		}

		mutableNode->staticMethodTarget = "";
		if (concreteTarget->isExternal)
		{

			if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			{ // Disable mangling for external functions
				id->name = concreteTarget->name;
			}
		}
		else if (isMethodCall || node->isConstructorCall)
		{
			mutableNode->staticMethodTarget = concreteTarget->name;
		}

		CallValidator::ValidateArguments(node, concreteTarget.get(), argTypes, isMethodCall);

		if (concreteTarget->returnType == nullptr)
		{
			throw std::runtime_error("Semantic Error: Cannot infer return type for forward call of '" + concreteTarget->name + "'.");
		}

		if (node->isConstructorCall)
		{
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(calleeType))
			{
				m_currentExprType = classType;
			}
			else
			{
				m_currentExprType = concreteTarget->paramTypes[0];
			}
		}
		else
		{
			m_currentExprType = concreteTarget->returnType;
		}
	}

	// ==========================================
	// INSTRUCTIONS
	// ==========================================

	void Visit(const ast::VarDecl* node) override
	{
		Declaration::Var(node->name, node->initializer.get(), node->type.get(), m_context);
	}

	void Visit(const ast::ValDecl* node) override
	{
		Declaration::Val(node->name, node->initializer.get(), node->type.get(), m_context);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		Evaluate(node->expr.get());
	}

	void Visit(const ast::ConstructorDecl* node) override
	{
		if (m_context.m_instantiatedNodes.contains(node))
		{
			return;
		}

		const Symbol* sym = m_context.env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

		m_context.env.PushScope();

		// Parameters including 'this'
		for (std::size_t i = 0; i < node->parameters.size(); ++i)
		{
			std::shared_ptr<SemanticType> paramType = funType->paramTypes[i];
			if (node->isVararg && i == node->parameters.size() - 1)
			{
				paramType = GetVarargArrayType(paramType);
			}
			m_context.env.Define(node->parameters[i].name, paramType, false);
		}

		const auto previousReturnType = m_context.location.currentReturnType;
		const auto previousFunctionType = m_context.location.currentFunction;

		m_context.location.currentReturnType = funType->returnType; // Unit
		m_context.location.currentFunction = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_context.location.currentReturnType = previousReturnType;
		m_context.location.currentFunction = previousFunctionType;
		m_context.env.PopScope();
	}

	void Visit(const ast::DestructorDecl* node) override
	{
		if (m_context.m_instantiatedNodes.contains(node))
		{
			return;
		}

		const Symbol* sym = m_context.env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

		m_context.env.PushScope();

		m_context.env.Define("this", funType->paramTypes[0], false);

		const auto previousReturnType = m_context.location.currentReturnType;
		const auto previousFunctionType = m_context.location.currentFunction;

		m_context.location.currentReturnType = funType->returnType; // Unit
		m_context.location.currentFunction = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_context.location.currentReturnType = previousReturnType;
		m_context.location.currentFunction = previousFunctionType;
		m_context.env.PopScope();
	}

	void Visit(const ast::AssignExpr* node) override
	{
		const auto targetType = Evaluate(node->target.get());
		const auto valueType = Evaluate(node->value.get());

		// Mutability check
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{ // val ident
			if (const Symbol* sym = m_context.env.Resolve(id->name); sym && sym->isReadOnly)
			{
				throw std::runtime_error("Semantic Error: Cannot reassign to read-only variable '" + id->name + "'");
			}
		}
		else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->target.get()))
		{ // ident.{val ident}
			const auto objectType = Evaluate(memAccess->object.get());
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(objectType))
			{
				if (const auto it = classType->fields.find(memAccess->member); it != classType->fields.end())
				{
					if (it->second.isReadOnly)
					{
						bool isInsideCtor = false;
						if (m_context.location.currentFunction)
						{ // allow val mutations in constructor
							const re::String expectedCtorName = classType->name + "_" + classType->name;
							if (m_context.location.currentFunction->name == expectedCtorName)
							{
								isInsideCtor = true;
							}
						}

						bool isThisAccess = false;
						if (const auto objId = dynamic_cast<const ast::IdentifierExpr*>(memAccess->object.get()))
						{ // allow this.val mutations
							if (objId->name.Hashed() == "this"_hs)
							{
								isThisAccess = true;
							}
						}

						if (!isInsideCtor || !isThisAccess)
						{
							throw std::runtime_error("Semantic Error: Cannot reassign read-only field '" + memAccess->member + "' of class '" + classType->name + "'");
						}
					}
				}
				else
				{
					throw std::runtime_error("Semantic Error: Cannot assign method '" + memAccess->member + "'");
				}
			}
		}

		TypeResolver::ExpectAssignable(valueType.get(), targetType.get(), "assignment");
		m_currentExprType = valueType;
	}

	void Visit(const ast::IfStmt* node) override
	{
		const auto condType = Evaluate(node->condition.get());
		TypeResolver::ExpectAssignable(condType.get(), m_context.tBool.get(), "if condition");

		if (node->thenBranch)
		{
			node->thenBranch->Accept(*this);
		}
		if (node->elseBranch)
		{
			node->elseBranch->Accept(*this);
		}
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const auto condType = Evaluate(node->condition.get());
		TypeResolver::ExpectAssignable(condType.get(), m_context.tBool.get(), "while condition");

		if (node->body)
		{
			node->body->Accept(*this);
		}
	}

	void Visit(const ast::ForStmt* node) override
	{
		m_context.env.PushScope();

		Evaluate(node->startExpr.get());
		Evaluate(node->endExpr.get());

		m_context.env.Define(node->iteratorName, m_context.tInt, false);

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_context.env.PopScope();
	}

	void Visit(const ast::Block* node) override
	{
		m_context.env.PushScope();
		const auto statementsCount = node->statements.size();
		for (std::size_t i = 0; i < statementsCount; ++i)
		{
			if (node->statements[i])
			{
				node->statements[i]->Accept(*this);
			}
		}
		m_context.env.PopScope();
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		const auto retType = Evaluate(node->expr.get());

		if (m_context.location.currentFunction && m_context.location.currentFunction->returnType == nullptr)
		{ // Resolving a type of return expression
			m_context.location.currentFunction->returnType = retType;
			m_context.location.currentReturnType = retType;
		}
		else if (m_context.location.currentReturnType)
		{
			TypeResolver::ExpectAssignable(retType.get(), m_context.location.currentReturnType.get(), "return statement");
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (m_context.m_instantiatedNodes.contains(node))
		{
			return;
		}

		if (!node->typeParams.empty())
		{ // Ignore generic function declarations
			return;
		}

		std::shared_ptr<FunctionType> funType;

		if (m_context.location.currentFunction != nullptr)
		{ // Local function
			funType = std::make_shared<FunctionType>(node->name);
			funType->returnType = TypeResolver::Resolve(node->returnType.get(), m_context);

			if (funType->returnType == nullptr && !node->isExprBody)
			{
				funType->returnType = m_context.tUnit;
			}

			funType->isVararg = node->isVararg;

			for (const auto& [_, type] : node->parameters)
			{
				funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
			}
			m_context.env.Define(node->name, funType, true);
		}
		else
		{ // Global function
			const Symbol* sym = m_context.env.Resolve(node->name);
			if (!sym)
			{
				throw std::runtime_error("Semantic Error: Cannot resolve '" + node->name + "'");
			}

			funType = std::dynamic_pointer_cast<FunctionType>(sym->type);
		}

		m_context.env.PushScope();
		for (std::size_t i = 0; i < node->parameters.size(); ++i)
		{
			std::shared_ptr<SemanticType> paramType = funType->paramTypes[i];
			if (node->isVararg && i == node->parameters.size() - 1)
			{
				paramType = GetVarargArrayType(paramType);
			}
			m_context.env.Define(node->parameters[i].name, paramType, false);
		}

		const auto previousReturnType = m_context.location.currentReturnType;
		const auto previousFunctionType = m_context.location.currentFunction;

		m_context.location.currentReturnType = funType->returnType;
		m_context.location.currentFunction = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		if (m_context.location.currentFunction->returnType == nullptr)
		{ // No return in function -> fallback to Unit
			m_context.location.currentFunction->returnType = m_context.tUnit;
		}

		m_context.location.currentReturnType = previousReturnType;
		m_context.location.currentFunction = previousFunctionType;
		m_context.env.PopScope();
	}

	void Visit(const ast::ClassDecl* node) override
	{
		if (m_context.m_instantiatedNodes.contains(node))
		{
			return;
		}

		if (!node->typeParams.empty())
		{ // Ignore generic classes declarations
			return;
		}

		std::shared_ptr<ClassType> classType;
		if (const Symbol* sym = m_context.env.Resolve(node->name))
		{
			classType = std::dynamic_pointer_cast<ClassType>(sym->type);
		}
		else if (m_context.instantiatedClasses.contains(node->name))
		{
			classType = m_context.instantiatedClasses.at(node->name);
		}
		else
		{
			throw std::runtime_error("Semantic Error: Class '" + node->name + "' not found during Visit");
		}

		const auto previousClass = m_context.location.currentClass;
		m_context.location.currentClass = classType;

		if (node->baseClass)
		{
			const auto baseType = TypeResolver::Resolve(node->baseClass->type.get(), m_context);
			classType->baseClass = std::dynamic_pointer_cast<ClassType>(baseType);

			if (!classType->baseClass)
			{
				throw std::runtime_error("Semantic Error: Base type must be a class");
			}

			for (const auto& [fieldName, fieldInfo] : classType->baseClass->fields)
			{ // Copying parent class fields in start of child class
				classType->fields[fieldName] = fieldInfo;
			}

			for (const auto& [methodName, methodType] : classType->baseClass->methods)
			{ // Copying parent class methods without overriding child class methods
				if (methodName != classType->baseClass->name && methodName[0] != '~')
				{
					if (!classType->methods.contains(methodName))
					{
						classType->methods[methodName] = methodType;
					} // --------------------------------------------------------
				}
			}
		}

		if (!classType->baseClass && classType->name != "Any")
		{ // Inherit every class from Any
			if (const Symbol* anySym = m_context.env.Resolve("Any"))
			{
				classType->baseClass = std::dynamic_pointer_cast<ClassType>(anySym->type);
			}
		}

		for (const auto& member : node->members)
		{ // Register member values
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{ // Member var
				auto fieldType = TypeResolver::Resolve(varDecl->type.get(), m_context);
				if (varDecl->initializer)
				{
					const auto initType = Evaluate(varDecl->initializer.get());
					if (!fieldType || fieldType == m_context.tUnit)
					{
						fieldType = initType;
					}
				}
				else if (!varDecl->isExternal && !node->isExternal && !fieldType)
				{
					throw std::runtime_error("Semantic Error: Property '" + varDecl->name + "' must have an initializer or explicit type");
				}

				classType->fields[varDecl->name] = { fieldType, false, varDecl->visibility };
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{ // Member val
				auto fieldType = TypeResolver::Resolve(valDecl->type.get(), m_context);
				if (valDecl->initializer)
				{
					const auto initType = Evaluate(valDecl->initializer.get());
					if (!fieldType || fieldType == m_context.tUnit)
					{
						fieldType = initType;
					}
				}
				else if (!valDecl->isExternal && !node->isExternal && !fieldType)
				{
					throw std::runtime_error("Semantic Error: Property '" + valDecl->name + "' must have an initializer or explicit type");
				}

				classType->fields[valDecl->name] = { fieldType, true, valDecl->visibility };
			}
		}

		for (const auto& member : node->members)
		{ // Register member functions
			if (const auto funDecl = dynamic_cast<const ast::FunDecl*>(member.get()))
			{
				re::String originalName = funDecl->name.Substring(node->name.Length() + 1);

				const bool existsInBase = classType->baseClass && classType->baseClass->methods.contains(originalName);
				if (funDecl->isOverride && !existsInBase)
				{
					throw std::runtime_error("Semantic Error: Method '" + originalName + "' is marked 'override' but no matching method found in base class");
				}
				if (!funDecl->isOverride && existsInBase)
				{
					throw std::runtime_error("Semantic Error: Method '" + originalName + "' hides base class method. Add the 'override' modifier.");
				}

				if (node->isExternal && (!funDecl->isExternal || funDecl->body || funDecl->isExprBody))
				{
					throw std::runtime_error("Semantic Error: Method '" + originalName + "' in external class '" + node->name + "' must be marked 'external' and cannot have a body.");
				}

				funDecl->Accept(*this);
			}
			else if (const auto ctorDecl = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
			{
				ctorDecl->Accept(*this);
			}
			else if (const auto dtorDecl = dynamic_cast<const ast::DestructorDecl*>(member.get()))
			{
				dtorDecl->Accept(*this);
			}
		}

		m_context.location.currentClass = previousClass;
	}

	void Visit(const ast::ImportDecl* node) override
	{
		Import::Process(node, m_context);
	}

private:
	SemanticContext m_context;

	std::shared_ptr<SemanticType> m_currentExprType = nullptr;
	ast::Program* m_currentProgram = nullptr;

	// --- Type Definitions ---

	// ==========================================
	// HELPER METHODS
	// ==========================================

	void InitBuiltins()
	{
		m_context.env.Define("Int", m_context.tInt, true);
		m_context.env.Define("Double", m_context.tDouble, true);
		m_context.env.Define("Bool", m_context.tBool, true);
		m_context.env.Define("String", m_context.tString, true);
		m_context.env.Define("Unit", m_context.tUnit, true);
		m_context.env.Define("Any", m_context.tAny, true);
	}

	std::shared_ptr<SemanticType> Evaluate(const ast::Expr* expr)
	{
		if (!expr)
		{
			return m_context.tUnit;
		}

		m_currentExprType = m_context.tUnit; // Reset state
		expr->Accept(*this);

		return m_currentExprType;
	}

	std::shared_ptr<SemanticType> GetVarargArrayType(const std::shared_ptr<SemanticType>& elementType)
	{
		if (const Symbol* sym = m_context.env.Resolve("Array"))
		{ // Instantiating Array class
			if (const auto tmpl = std::dynamic_pointer_cast<GenericClassTemplate>(sym->type))
			{
				return m_context.instantiateClassCallback(tmpl, { elementType });
			}

			return std::dynamic_pointer_cast<SemanticType>(sym->type);
		}

		return m_context.tUnit; // Fallback in case stdlib is not loaded
	}

	void RegisterProgramDeclarations(const ast::Program* node)
	{
		const bool isGlobal = node->packageName.Empty() || node->packageName == "global";
		std::shared_ptr<ModuleType> currentModule = nullptr;

		if (!isGlobal)
		{
			if (const Symbol* sym = m_context.env.Resolve(node->packageName))
			{
				currentModule = std::dynamic_pointer_cast<ModuleType>(sym->type);
			}

			if (!currentModule)
			{
				currentModule = std::make_shared<ModuleType>(node->packageName);
				m_context.env.Define(node->packageName, currentModule, true);
			}
		}

		m_context.location.currentPackage = currentModule;
		const std::size_t originalStmtCount = node->statements.size();

		for (std::size_t i = 0; i < originalStmtCount; ++i)
		{
			ast::Statement* stmt = node->statements[i].get();
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt))
			{
				if (!classDecl->typeParams.empty())
				{ // Generic non-instantiated class
					auto tmpl = std::make_shared<GenericClassTemplate>(classDecl->name);
					tmpl->astNode = classDecl;
					tmpl->typeParams = classDecl->typeParams;
					tmpl->moduleName = currentModule ? currentModule->name : "global";

					if (isGlobal)
					{
						m_context.env.Define(classDecl->name, tmpl, true);
					}
					else
					{
						currentModule->exports[classDecl->name] = tmpl;
					}

					continue;
				}

				// Non-generic or instantiated class
				std::shared_ptr<ClassType> classType = nullptr;
				bool isNewClass = false;

				if (isGlobal)
				{
					if (const Symbol* existingSym = m_context.env.Resolve(classDecl->name))
					{
						classType = std::dynamic_pointer_cast<ClassType>(existingSym->type);
					}
				}

				if (!classType)
				{ // New previously undefined class
					classType = std::make_shared<ClassType>(classDecl->name);
					isNewClass = true;
				}
				else
				{ // Extending already defined class
					isNewClass = false;
				}

				m_context.allClassTypes[classDecl->name] = classType;

				bool hasConstructor = false;
				for (const auto& member : classDecl->members)
				{
					if (dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						hasConstructor = true;
						break;
					}
				}

				if (!hasConstructor)
				{
					auto defaultCtor = std::make_unique<ast::ConstructorDecl>();
					defaultCtor->name = classDecl->name;
					defaultCtor->body = std::make_unique<ast::Block>();

					const_cast<ast::ClassDecl*>(classDecl)->members.push_back(std::move(defaultCtor));
				}

				classType->moduleName = currentModule ? currentModule->name : "global";

				if (isGlobal && isNewClass)
				{
					m_context.env.Define(classDecl->name, classType, true);
				}
				else if (!isGlobal)
				{
					currentModule->exports[classDecl->name] = classType;
				}

				for (const auto& member : classDecl->members)
				{
					if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
					{
						const auto mutableFun = const_cast<ast::FunDecl*>(fun);
						const re::String originalName = mutableFun->name;

						if (mutableFun->parameters.empty() || mutableFun->parameters[0].name != "this")
						{
							ast::FunDecl::Parameter thisParam;
							thisParam.name = "this";
							auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
							thisTypeNode->name = classDecl->name;
							thisParam.type = std::move(thisTypeNode);
							mutableFun->parameters.insert(mutableFun->parameters.begin(), std::move(thisParam));
						}

						mutableFun->name = classDecl->name + "_" + originalName;
						m_context.allFunctionNames.insert(mutableFun->name);

						if (!mutableFun->typeParams.empty())
						{
							if (mutableFun->isOverride)
							{
								throw std::runtime_error("Semantic Error: Generic methods cannot be marked 'override' (restricted by monomorphization). Method: '" + originalName + "'");
							}

							auto tmpl = std::make_shared<GenericFunctionTemplate>(mutableFun->name);
							tmpl->astNode = mutableFun;
							tmpl->typeParams = mutableFun->typeParams;
							tmpl->moduleName = currentModule ? currentModule->name : "global";

							tmpl->visibility = mutableFun->visibility;
							tmpl->isExternal = fun->isExternal || classType->isExternal;

							classType->methods[originalName] = tmpl;
							continue;
						}

						auto funType = std::make_shared<FunctionType>(mutableFun->name);
						funType->returnType = TypeResolver::Resolve(mutableFun->returnType.get(), m_context);
						if (funType->returnType == nullptr && !mutableFun->isExprBody)
						{
							funType->returnType = m_context.tUnit;
						}
						funType->isVararg = mutableFun->isVararg;

						for (const auto& [_, type] : mutableFun->parameters)
						{
							funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
						}

						funType->visibility = mutableFun->visibility;
						funType->moduleName = currentModule ? currentModule->name : "global";
						funType->isExternal = fun->isExternal || classDecl->isExternal;

						if (isGlobal)
						{
							m_context.env.Define(mutableFun->name, funType, true);
						}
						else
						{
							currentModule->exports[mutableFun->name] = funType;
						}

						classType->methods[originalName] = funType;
					}
					else if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						const auto mutableCtor = const_cast<ast::ConstructorDecl*>(ctor);

						ast::FunDecl::Parameter thisParam;
						thisParam.name = "this";
						auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
						thisTypeNode->name = classDecl->name;
						thisParam.type = std::move(thisTypeNode);
						mutableCtor->parameters.insert(mutableCtor->parameters.begin(), std::move(thisParam));

						mutableCtor->name = classDecl->name + "_" + classDecl->name;
						m_context.allFunctionNames.insert(mutableCtor->name);

						auto funType = std::make_shared<FunctionType>(mutableCtor->name);
						funType->returnType = m_context.tUnit;
						funType->isVararg = mutableCtor->isVararg;
						funType->isExternal = classDecl->isExternal;

						for (const auto& [_, type] : mutableCtor->parameters)
						{
							funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
						}

						funType->visibility = mutableCtor->visibility;

						if (isGlobal)
						{
							m_context.env.Define(mutableCtor->name, funType, true);
						}
						else
						{
							currentModule->exports[mutableCtor->name] = funType;
						}

						classType->methods[classDecl->name] = funType;
					}
					else if (const auto dtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
					{
						const auto mutableDtor = const_cast<ast::DestructorDecl*>(dtor);

						mutableDtor->name = classDecl->name + "_destructor";
						m_context.allFunctionNames.insert(mutableDtor->name);

						auto funType = std::make_shared<FunctionType>(mutableDtor->name);
						funType->returnType = m_context.tUnit;

						auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
						thisTypeNode->name = classDecl->name;
						funType->paramTypes.emplace_back(TypeResolver::Resolve(thisTypeNode.get(), m_context));

						funType->visibility = mutableDtor->visibility;
						funType->isExternal = classDecl->isExternal;

						if (isGlobal)
						{
							m_context.env.Define(mutableDtor->name, funType, true);
						}
						else
						{
							currentModule->exports[mutableDtor->name] = funType;
						}

						classType->methods["~" + classDecl->name] = funType;
					}
				}
			}

			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt))
			{
				if (!fun->typeParams.empty())
				{ // Generic non-instantiated function
					auto tmpl = std::make_shared<GenericFunctionTemplate>(fun->name);
					tmpl->astNode = fun;
					tmpl->typeParams = fun->typeParams;
					tmpl->moduleName = currentModule ? currentModule->name : "global";

					tmpl->visibility = fun->visibility;
					tmpl->isExternal = fun->isExternal;

					if (isGlobal)
					{
						m_context.env.Define(fun->name, tmpl, true);
					}
					else
					{
						currentModule->exports[fun->name] = tmpl;
					}

					if (fun->isExternal)
					{
						m_context.externalFunctions.insert(fun->name);
					}

					continue;
				}

				// Non-generic or instantiated function
				m_context.allFunctionNames.insert(fun->name);

				auto funType = std::make_shared<FunctionType>(fun->name);
				funType->returnType = TypeResolver::Resolve(fun->returnType.get(), m_context);

				if (funType->returnType == nullptr && !fun->isExprBody)
				{
					funType->returnType = m_context.tUnit;
				}

				funType->isVararg = fun->isVararg;

				for (const auto& [_, type] : fun->parameters)
				{
					funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
				}

				funType->visibility = fun->visibility;
				funType->moduleName = currentModule ? currentModule->name : "global";
				funType->isExternal = fun->isExternal;

				if (isGlobal)
				{
					m_context.env.Define(funType->name, funType, true);
				}
				else
				{
					currentModule->exports[funType->name] = funType;
				}

				if (fun->isExternal)
				{
					m_context.externalFunctions.insert(fun->name);
				}
			}
		}

		m_context.location.currentPackage = nullptr;
	}
};

} // namespace igni::sem