#pragma once

#include "Helpers/Declaration/Overload.hpp"

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/Helpers/CallValidator.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Variable.hpp>
#include <IgniLang/Semantic/Helpers/Export.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Class.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Function.hpp>
#include <IgniLang/Semantic/Helpers/Import.hpp>
#include <IgniLang/Semantic/Helpers/MemberResolver.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
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
		case TokenType::KwNull:      m_currentExprType = m_context.tNull; break;
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
				IGNI_SEM_ERR("Cannot use 'super' outside of a derived class");
			}
			m_currentExprType = m_context.location.currentClass->baseClass;

			return;
		}

		const Symbol* sym = m_context.env.Resolve(node->name);
		if (!sym)
		{
			IGNI_SEM_ERR("Undefined variable '" + node->name + "'");
		}

		if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(sym->type))
		{
			if (group->overloads.size() == 1 && group->templates.empty())
			{
				m_currentExprType = group->overloads.front();

				const_cast<ast::IdentifierExpr*>(node)->name = group->overloads.front()->isExternal
					? group->name
					: group->overloads.front()->name;

				return;
			}
		}

		m_currentExprType = sym->type;
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		const auto leftType = Evaluate(node->object.get());

		if (leftType->isNullable && !node->isSafe)
		{
			IGNI_SEM_ERR("Cannot perform unsafe member access of nullable type '" + leftType->name + "'");
		}

		if (const auto modType = std::dynamic_pointer_cast<ModuleType>(leftType))
		{
			m_currentExprType = Export::Resolve(modType, node);
		}
		else if (const auto classType = std::dynamic_pointer_cast<ClassType>(leftType))
		{
			m_currentExprType = MemberResolver::ResolveAccess(classType, node->member, m_context);

			if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(m_currentExprType))
			{
				if (group->overloads.size() == 1 && group->templates.empty())
				{
					m_currentExprType = group->overloads.front();

					const_cast<ast::MemberAccessExpr*>(node)->member = group->overloads.front()->isExternal
						? group->name
						: group->overloads.front()->name;
				}
			}
		}
		else
		{
			IGNI_SEM_ERR("Cannot access member '" + node->member + "' on primitive type '" + leftType->name + "'");
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		const auto leftType = Evaluate(node->left.get());
		const auto rightType = Evaluate(node->right.get());

		std::shared_ptr<SemanticType> mathType = TypeResolver::PromoteMathTypes(leftType.get(), rightType.get(), node, m_context);
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

		TypeResolver::ExpectAssignable(indexType.get(), m_context.tInt.get(), "array index", node);

		if (const auto classType = std::dynamic_pointer_cast<ClassType>(arrType))
		{ // allow index access operation to any class with get(Int) method
			if (const auto it = classType->methods.find("get"); it != classType->methods.end())
			{
				if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(it->second))
				{
					const std::vector argTypes = { indexType };
					std::shared_ptr<FunctionType> bestMatch = nullptr;
					int matchCount = 0;

					for (const auto& overload : group->overloads)
					{
						if (CallValidator::IsApplicable(overload.get(), argTypes, true))
						{
							bestMatch = overload;
							matchCount++;
						}
					}

					if (matchCount == 1)
					{
						m_currentExprType = bestMatch->returnType;
						return;
					}
					if (matchCount > 1)
					{
						IGNI_SEM_ERR(node, "Ambiguous indexing: multiple matching 'get' overloads found for type '" + classType->name + "'");
					}
				}
			}
		}

		IGNI_SEM_ERR("Type '" + arrType->name + "' does not support indexing (missing 'get' method)");
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

		auto resolveOverload = [&](const std::shared_ptr<FunctionGroup>& group, bool isMethod) -> std::shared_ptr<FunctionType> {
			std::shared_ptr<FunctionType> bestMatch = nullptr;
			int matchCount = 0;
			for (const auto& overload : group->overloads)
			{
				if (CallValidator::IsApplicable(overload.get(), argTypes, isMethod))
				{
					bestMatch = overload;
					matchCount++;
				}
			}
			if (matchCount > 1)
			{
				IGNI_SEM_ERR(node, "Ambiguous call: multiple matching overloads found");
			}
			return bestMatch;
		};
		// ==========================================

		if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(calleeType))
		{ // Generic class constructor
			if (explicitTypeArgs.empty())
			{
				const ast::ConstructorDecl* ctorAst = nullptr;
				for (const auto& member : classTmpl->astNode->members)
				{ // Finding constructor in class members
					if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{ // TODO: Add generics overload for constructors
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
							IGNI_SEM_ERR("Could not infer type arguments for generic class '" + classTmpl->name + "'. Use explicit ::<T> syntax.");
						}
					}
				}
				else
				{
					IGNI_SEM_ERR("Generic class '" + classTmpl->name + "' has no parameters to infer types from. Use explicit ::<T> syntax.");
				}
			}

			const auto concreteClass = m_context.instantiateClassCallback(classTmpl, explicitTypeArgs);
			const auto it = concreteClass->methods.find(concreteClass->name);
			if (it == concreteClass->methods.end())
			{
				IGNI_SEM_ERR("Constructor not found for generic class '" + concreteClass->name + "'");
			}

			if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(it->second))
			{
				concreteTarget = resolveOverload(group, true);
			}

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
				IGNI_SEM_ERR("Constructor not found for class '" + classType->name + "'");
			}

			if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(it->second))
			{
				concreteTarget = resolveOverload(group, true);
			}

			const_cast<ast::CallExpr*>(node)->isConstructorCall = true;
			isMethodCall = true;
		}
		else if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(calleeType))
		{ // Function or Method call overloads
			isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr;
			concreteTarget = resolveOverload(group, isMethodCall);

			if (!concreteTarget && !group->templates.empty())
			{
				// TODO: Add generic function instantiation like SFINAE
				concreteTarget = CallValidator::ResolveAndInstantiateGeneric(group->templates[0], explicitTypeArgs, argTypes, m_context);
			}

			if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			{
				if (concreteTarget)
				{
					id->name = concreteTarget->isExternal ? group->name : concreteTarget->name;
				}
			}
			else if (const auto memAccess = dynamic_cast<ast::MemberAccessExpr*>(node->callee.get()))
			{
				if (concreteTarget)
				{
					memAccess->member = concreteTarget->isExternal ? group->name : concreteTarget->name;
				}
			}
		}
		else if (const auto funType = std::dynamic_pointer_cast<FunctionType>(calleeType))
		{ // Non-generic function fallback (e.g. lambda variables)
			isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr;
			concreteTarget = funType;
		}
		else
		{
			IGNI_SEM_ERR("Attempt to call a non-callable type");
		}

		const auto mutableNode = const_cast<ast::CallExpr*>(node);
		mutableNode->isSuperCall = isSuperCall;

		if (!concreteTarget)
		{
			IGNI_SEM_ERR(node, "No matching overload found for call to '" + re::String(node->callee ? node->callee->token.lexeme : "unknown") + "'");
		}

		if (isSuperCall)
		{ // super(Args...) constructor call
			auto thisExpr = std::make_unique<ast::IdentifierExpr>();
			thisExpr->name = "this";
			thisExpr->token = node->token;

			const auto thisType = Evaluate(thisExpr.get());

			mutableNode->arguments.insert(mutableNode->arguments.begin(), std::move(thisExpr));
			argTypes.insert(argTypes.begin(), thisType);

			mutableNode->isConstructorCall = false;
			isMethodCall = false;
			mutableNode->staticMethodTarget = concreteTarget->name;

			if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			{
				id->name = concreteTarget->name;
			}
		}

		if (concreteTarget->isExternal)
		{ // Keep external function original name for code generator
		}
		else if (isMethodCall || node->isConstructorCall)
		{
			mutableNode->staticMethodTarget = concreteTarget->name;
		}

		CallValidator::ValidateArguments(node, concreteTarget.get(), argTypes, isMethodCall);

		if (concreteTarget->returnType == nullptr)
		{
			IGNI_SEM_ERR("Cannot infer return type for forward call of '" + concreteTarget->name + "'.");
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
		std::shared_ptr<FunctionType> funType;

		if (m_context.instantiatedFunctions.contains(node->name))
		{
			funType = m_context.instantiatedFunctions.at(node->name);
		}

		if (!funType)
		{
			IGNI_SEM_ERR("Cannot resolve constructor '" + node->name + "'");
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
		else if (!node->isExternal)
		{
			IGNI_SEM_ERR("Non-external constructor must have a body");
		}

		m_context.location.currentReturnType = previousReturnType;
		m_context.location.currentFunction = previousFunctionType;
		m_context.env.PopScope();
	}

	void Visit(const ast::DestructorDecl* node) override
	{
		std::shared_ptr<FunctionType> funType;

		if (m_context.instantiatedFunctions.contains(node->name))
		{
			funType = m_context.instantiatedFunctions.at(node->name);
		}

		if (!funType)
		{
			IGNI_SEM_ERR("Cannot resolve destructor '" + node->name + "'");
		}

		m_context.env.PushScope();
		m_context.env.Define("this", funType->paramTypes[0], false);

		const auto previousReturnType = m_context.location.currentReturnType;
		const auto previousFunctionType = m_context.location.currentFunction;

		m_context.location.currentReturnType = funType->returnType;
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
				IGNI_SEM_ERR("Cannot reassign to read-only variable '" + id->name + "'");
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
							IGNI_SEM_ERR("Cannot reassign read-only field '" + memAccess->member + "' of class '" + classType->name + "'");
						}
					}
				}
				else
				{
					IGNI_SEM_ERR("Semantic Error: Cannot assign method '" + memAccess->member + "'");
				}
			}
		}
		else if (const auto idxAccess = dynamic_cast<const ast::IndexExpr*>(node->target.get()))
		{
			const auto arrType = Evaluate(idxAccess->array.get());
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(arrType))
			{
				if (!classType->methods.contains("set"))
				{
					IGNI_SEM_ERR("Type '" + classType->name + "' does not support indexed assignment (missing 'set' method)");
				}
			}
		}

		TypeResolver::ExpectAssignable(valueType.get(), targetType.get(), "assignment", node);
		m_currentExprType = valueType;
	}

	void Visit(const ast::IfStmt* node) override
	{
		const auto condType = Evaluate(node->condition.get());
		TypeResolver::ExpectAssignable(condType.get(), m_context.tBool.get(), "if condition", node);

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
		TypeResolver::ExpectAssignable(condType.get(), m_context.tBool.get(), "while condition", node);

		if (node->body)
		{
			node->body->Accept(*this);
		}
	}

	void Visit(const ast::ForStmt* node) override
	{
		m_context.env.PushScope();

		if (!node->isForEach)
		{
			Evaluate(node->startExpr.get());
			Evaluate(node->endExpr.get());

			m_context.env.Define(node->iteratorName, m_context.tInt, false);
		}
		else
		{
			const auto collectionType = Evaluate(node->startExpr.get());
			std::shared_ptr<SemanticType> elementType = nullptr;

			if (const auto classType = std::dynamic_pointer_cast<ClassType>(collectionType))
			{
				if (classType->name.Find("Array") != re::String::NPos && !classType->typeArguments.empty())
				{
					elementType = classType->typeArguments[0];
				}
			}

			if (!elementType)
			{
				IGNI_SEM_ERR("For-each loop requires an Array collection, but got '" + collectionType->name + "'");
			}

			m_context.env.Define(node->iteratorName, elementType, false);
		}

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
			TypeResolver::ExpectAssignable(retType.get(), m_context.location.currentReturnType.get(), "return statement", node);
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (!node->typeParams.empty() || node->isExternal)
		{ // Ignore generic and external functions
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
		{ // Global function or Class method
			if (m_context.instantiatedFunctions.contains(node->name))
			{
				funType = m_context.instantiatedFunctions.at(node->name);
			}

			if (!funType)
			{
				IGNI_SEM_ERR(node, "Cannot resolve function '" + node->name + "'");
			}
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
			IGNI_SEM_ERR("Class '" + node->name + "' not found during Visit");
		}

		const auto previousClass = m_context.location.currentClass;
		m_context.location.currentClass = classType;

		if (node->baseClass)
		{
			const auto baseType = TypeResolver::Resolve(node->baseClass->type.get(), m_context);
			classType->baseClass = std::dynamic_pointer_cast<ClassType>(baseType);

			if (!classType->baseClass)
			{
				IGNI_SEM_ERR("Base type must be a class");
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
					IGNI_SEM_ERR(varDecl, "Property '" + varDecl->name + "' must have an initializer or explicit type");
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
					IGNI_SEM_ERR(valDecl, "Property '" + valDecl->name + "' must have an initializer or explicit type");
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
					IGNI_SEM_ERR(member.get(), "Method '" + originalName + "' is marked 'override' but no matching method found in base class");
				}
				if (!funDecl->isOverride && existsInBase)
				{
					IGNI_SEM_ERR(member.get(), "Method '" + originalName + "' hides base class method. Add the 'override' modifier.");
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
					classType = std::make_shared<ClassType>(classDecl->name, classDecl);
					isNewClass = true;
				}
				else
				{ // Extending already defined class
					isNewClass = false;
				}

				// clang-format off
				switch (classDecl->name.Hashed())
				{
				case "String"_hs: m_context.tString = classType; break;
				case "Int"_hs:    m_context.tInt = classType; break;
				case "Double"_hs: m_context.tDouble = classType; break;
				case "Bool"_hs:   m_context.tBool = classType; break;
				case "Any"_hs:    m_context.tAny = classType; break;
				default: break;
				}
				// clang-format on

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
					if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
					{
						if (varDecl->type)
						{
							classType->fields[varDecl->name] = { TypeResolver::Resolve(varDecl->type.get(), m_context), false, varDecl->visibility };
						}
					}
					else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
					{
						if (valDecl->type)
						{
							classType->fields[valDecl->name] = { TypeResolver::Resolve(valDecl->type.get(), m_context), true, valDecl->visibility };
						}
					}
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
						const re::String originalName = Declaration::InjectThisKeyword(mutableFun, classDecl->name);

						if (!mutableFun->typeParams.empty())
						{
							if (mutableFun->isOverride)
							{
								IGNI_SEM_ERR(mutableFun, "Generic methods cannot be marked 'override'");
							}

							auto tmpl = Declaration::GenericFunction(mutableFun, currentModule ? currentModule->name : "global");
							Declaration::Overload::GenericMethod(classType, originalName, tmpl);
							continue;
						}

						auto funType = Declaration::Method(mutableFun, originalName, classType, m_context);
						Declaration::Overload::Method(classType, originalName, funType);
					}
					else if (const auto ctor = dynamic_cast<ast::ConstructorDecl*>(member.get()))
					{
						auto funType = Declaration::Constructor(ctor, classType, classDecl->isExternal, m_context);
						Declaration::Overload::Method(classType, classDecl->name, funType);
					}
					else if (const auto dtor = dynamic_cast<ast::DestructorDecl*>(member.get()))
					{
						auto funType = Declaration::Destructor(dtor, classType, classDecl->isExternal, m_context);
						Declaration::Overload::Method(classType, "~" + classDecl->name, funType);
					}
				}
			}

			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt))
			{
				const re::String originalName = fun->name;

				if (!fun->typeParams.empty())
				{ // Generic non-instantiated function
					auto tmpl = Declaration::GenericFunction(const_cast<ast::FunDecl*>(fun), currentModule ? currentModule->name : "global");
					if (fun->isExternal)
					{
						m_context.externalFunctions.insert(originalName);
					}

					Declaration::Overload::GenericGlobal(originalName, tmpl, m_context, currentModule);
					continue;
				}

				// Non-generic or instantiated function
				auto funType = Declaration::Function(const_cast<ast::FunDecl*>(fun), m_context, currentModule ? currentModule->name : "global");
				Declaration::Overload::Global(originalName, funType, m_context, currentModule);
			}
		}

		m_context.location.currentPackage = nullptr;
	}
};

} // namespace igni::sem