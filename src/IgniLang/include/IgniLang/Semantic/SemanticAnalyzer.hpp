#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/Helpers/CallResolver.hpp>
#include <IgniLang/Semantic/Helpers/CallValidator.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Overload.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Variable.hpp>
#include <IgniLang/Semantic/Helpers/Export.hpp>
#include <IgniLang/Semantic/Helpers/FunctionAnalyzer.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Class.hpp>
#include <IgniLang/Semantic/Helpers/Generic/Function.hpp>
#include <IgniLang/Semantic/Helpers/Import.hpp>
#include <IgniLang/Semantic/Helpers/MemberResolver.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <GeneratedSemantics.hpp>

#include <algorithm>
#include <complex>

namespace igni::sem
{

using namespace re::literals;

class SemanticAnalyzer final : public ast::BaseAstVisitor
{
public:
	explicit SemanticAnalyzer(const generated::TargetConfig& config)
		: m_targetConfig(config)
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

		m_context.ffiAnnotations = m_targetConfig.ffiAnnotations;

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
					try
					{
						prog->statements[i]->Accept(*this);
					}
					catch (const SemanticBailoutException&)
					{
					}
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

	[[nodiscard]] const BindingContext& GetBindings() const
	{
		return m_context.bindings;
	}

	const std::vector<const ast::LambdaExpr*>& GetLambdas() const
	{
		return m_context.allLambdas;
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

		bool isImplicitThis = false;
		std::shared_ptr<SemanticType> exprType = nullptr;

		const Symbol* sym = m_context.env.Resolve(node->name);
		const bool isGlobal = sym && GetGlobalNames().contains(node->name);
		if (m_context.location.currentClass)
		{
			auto curr = m_context.location.currentClass;
			while (curr)
			{
				if (auto it = curr->fields.find(node->name); it != curr->fields.end())
				{
					if (!sym || isGlobal)
					{
						isImplicitThis = true;
						exprType = it->second.type;
					}
					break;
				}
				if (auto it = curr->methods.find(node->name); it != curr->methods.end())
				{
					if (!sym || isGlobal)
					{
						isImplicitThis = true;
						exprType = it->second;
					}
					break;
				}
				curr = curr->baseClass;
			}
		}

		if (isImplicitThis)
		{
			m_context.bindings.implicitThisNames.insert(node);
		}
		else
		{
			if (!sym)
			{
				IGNI_SEM_ERR("Undefined variable '" + node->name + "'");
			}
			exprType = sym->type;
		}

		if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(exprType))
		{
			if (group->overloads.size() == 1 && group->templates.empty())
			{
				m_currentExprType = group->overloads.front();

				m_context.bindings.resolvedNames[node] = group->overloads.front()->isExternal
					? group->name
					: group->overloads.front()->name;

				return;
			}
		}

		m_currentExprType = exprType;
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

					m_context.bindings.resolvedMembers[node] = group->overloads.front()->isExternal
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

		const re::String readMethod = generated::GetDesugaredMethod("IndexExpr", false);
		if (const auto classType = std::dynamic_pointer_cast<ClassType>(arrType))
		{ // allow index access operation to any class with get(T) method
			if (const auto it = classType->methods.find(readMethod); it != classType->methods.end())
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
		m_currentExprType = CallResolver::Process(node, m_context);
	}

	// ==========================================
	// INSTRUCTIONS
	// ==========================================

	void Visit(const ast::VarDecl* node) override
	{
		ValidateAnnotations(node);

		Declaration::Var(node->name, node->initializer.get(), node->type.get(), m_context);
	}

	void Visit(const ast::ValDecl* node) override
	{
		ValidateAnnotations(node);

		Declaration::Val(node->name, node->initializer.get(), node->type.get(), m_context);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		Evaluate(node->expr.get());
	}

	void Visit(const ast::ConstructorDecl* node) override
	{
		ValidateAnnotations(node);
		const re::String lookupName = m_context.bindings.funMeta.contains(node)
			? m_context.bindings.funMeta.at(node).mangledName
			: node->name;

		const auto funType = m_context.instantiatedFunctions.at(lookupName);
		if (!funType)
		{
			IGNI_SEM_ERR("Cannot resolve constructor '" + node->name + "'");
		}

		if (!node->body && !node->isExternal)
		{
			IGNI_SEM_ERR("Non-external constructor must have a body");
		}

		FunctionAnalyzer::AnalyzeBody(*this, m_context, funType, node->parameters, node->body.get(), true, m_context.location.currentClass);
	}

	void Visit(const ast::DestructorDecl* node) override
	{
		ValidateAnnotations(node);

		const re::String lookupName = m_context.bindings.funMeta.contains(node)
			? m_context.bindings.funMeta.at(node).mangledName
			: node->name;

		const auto funType = m_context.instantiatedFunctions.at(lookupName);
		if (!funType)
		{
			IGNI_SEM_ERR("Cannot resolve destructor '" + node->name + "'");
		}

		FunctionAnalyzer::AnalyzeBody(*this, m_context, funType, {}, node->body.get(), true, m_context.location.currentClass);
	}

	void Visit(const ast::AssignExpr* node) override
	{
		const auto targetType = Evaluate(node->target.get());
		const auto valueType = Evaluate(node->value.get());

		// Mutability check
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			if (m_context.bindings.implicitThisNames.contains(id))
			{
				auto curr = m_context.location.currentClass;
				while (curr)
				{
					if (auto it = curr->fields.find(id->name); it != curr->fields.end())
					{
						if (it->second.isReadOnly)
						{
							bool isInsideCtor = false;
							if (m_context.location.currentFunction)
							{ // allow val mutations in constructor
								if (const re::String expectedCtorName = m_context.location.currentClass->name + "_" + m_context.location.currentClass->name;
									m_context.location.currentFunction->name.Find(expectedCtorName) != re::String::NPos)
								{
									isInsideCtor = true;
								}
							}
							if (!isInsideCtor)
							{
								IGNI_SEM_ERR(node, "Cannot reassign read-only implicit field '" + id->name + "' of class '" + m_context.location.currentClass->name + "'");
							}
						}
						break;
					}
					curr = curr->baseClass;
				}
			}
			else if (const Symbol* sym = m_context.env.Resolve(id->name); sym && sym->isReadOnly)
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
							if (const re::String expectedCtorName = classType->name + "_" + classType->name;
								m_context.location.currentFunction->name.Find(expectedCtorName) != re::String::NPos)
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
			const auto writeMethod = generated::GetDesugaredMethod("IndexExpr", true);

			if (const auto classType = std::dynamic_pointer_cast<ClassType>(arrType))
			{
				if (!classType->methods.contains(writeMethod))
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
		if (node->isCompileTime)
		{
			const bool conditionResult = EvaluateConstExpr(node->condition.get());

			const auto mutableNode = const_cast<ast::IfStmt*>(node);
			if (conditionResult)
			{
				mutableNode->elseBranch = nullptr;
				if (node->thenBranch)
				{
					node->thenBranch->Accept(*this);
				}
			}
			else
			{
				mutableNode->thenBranch = nullptr;
				if (node->elseBranch)
				{
					node->elseBranch->Accept(*this);
				}
			}
			return;
		}

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
				try
				{
					node->statements[i]->Accept(*this);
				}
				catch (const SemanticBailoutException&)
				{
				}
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
		ValidateAnnotations(node);

		if (!node->typeParams.empty() || node->isExternal)
		{ // Ignore generic and external functions
			return;
		}

		std::shared_ptr<FunctionType> funType;
		bool isMethod = false;
		std::shared_ptr<ClassType> parentClass = nullptr;

		if (m_context.location.currentFunction != nullptr)
		{ // Local function
			funType = std::make_shared<FunctionType>(node->name);
			funType->isSuspend = node->isSuspend;
			funType->returnType = TypeResolver::Resolve(node->returnType.get(), m_context);
			funType->isVararg = node->isVararg;

			for (const auto& [_, type] : node->parameters)
			{
				funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
			}
			m_context.env.Define(node->name, funType, true);
		}
		else
		{ // Global function or Class method
			const re::String lookupName = m_context.bindings.funMeta.contains(node)
				? m_context.bindings.funMeta.at(node).mangledName
				: node->name;

			funType = m_context.instantiatedFunctions.at(lookupName);
			if (!funType)
			{
				IGNI_SEM_ERR(node, "Cannot resolve function '" + node->name + "'");
			}

			if (m_context.bindings.funMeta.contains(node))
			{
				const auto& meta = m_context.bindings.funMeta.at(node);
				isMethod = meta.isMethod;
				if (isMethod)
				{
					parentClass = GetClassType(meta.parentClass);
				}
			}
		}

		FunctionAnalyzer::AnalyzeBody(*this, m_context, funType, node->parameters, node->body.get(), isMethod, parentClass);
	}

	void Visit(const ast::ClassDecl* node) override
	{
		ValidateAnnotations(node);

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
					}
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

		m_context.env.PushScope();
		m_context.env.Define("this", classType, false);

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

		m_context.env.PopScope();

		for (const auto& member : node->members)
		{ // Register member functions
			if (const auto funDecl = dynamic_cast<const ast::FunDecl*>(member.get()))
			{
				re::String originalName = funDecl->name;

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

	void Visit(const ast::AwaitExpr* node) override
	{
		if (!m_context.location.currentFunction || !m_context.location.currentFunction->isSuspend)
		{
			IGNI_SEM_ERR(node, "The 'await' operator can only be used inside a 'suspend' function.");
		}

		// TODO: Add Task<T> end extract generic argument
		m_currentExprType = Evaluate(node->expression.get());
	}

	void Visit(const ast::LaunchExpr* node) override
	{
		if (!dynamic_cast<const ast::CallExpr*>(node->callable.get()) && !dynamic_cast<const ast::IdentifierExpr*>(node->callable.get()))
		{
			IGNI_SEM_ERR(node, "The 'launch' operator expects a function call.");
		}

		const bool prevContext = m_context.location.isInsideLaunch;
		m_context.location.isInsideLaunch = true;

		Evaluate(node->callable.get());

		m_context.location.isInsideLaunch = prevContext;

		// TODO: Return Job type
		m_currentExprType = m_context.tUnit;
	}

	void Visit(const ast::TypeCastExpr* node) override
	{
		const auto srcType = Evaluate(node->expr.get());

		const auto dstType = TypeResolver::Resolve(node->targetType.get(), m_context);
		if (!dstType)
		{
			IGNI_SEM_ERR(node, "Unknown target type for cast");
		}

		// TODO: Add type casting correctness checks (e.g. String to Int -> Error)

		m_context.bindings.castTargets[node] = dstType->name;

		m_currentExprType = dstType;
	}

	void Visit(const ast::LambdaExpr* node) override
	{
		re::String lambdaName = "__lambda_" + std::to_string(m_context.lambdaCounter++);

		auto funType = std::make_shared<FunctionType>(lambdaName);
		funType->isVararg = node->isVararg;

		// Формируем типы параметров
		for (const auto& [name, type] : node->parameters)
		{
			funType->paramTypes.push_back(TypeResolver::Resolve(type.get(), m_context));
		}
		if (node->returnType)
		{
			funType->returnType = TypeResolver::Resolve(node->returnType.get(), m_context);
		}

		auto injectCaptures = [&] {
			for (const auto& cap : node->captures)
			{
				const Symbol* sym = m_context.env.Resolve(cap);
				if (!sym)
				{
					IGNI_SEM_ERR(node, "Cannot capture undefined variable '" + cap + "'");
				}
				m_context.env.Define(cap, sym->type, false);
			}
		};

		FunctionAnalyzer::AnalyzeBody(*this,
			m_context, funType,
			node->parameters, node->body.get(),
			false, nullptr, injectCaptures);

		m_context.bindings.lambdaMeta[node] = { lambdaName };
		m_context.allLambdas.emplace_back(node);
		m_currentExprType = funType;
	}

private:
	SemanticContext m_context;
	generated::TargetConfig m_targetConfig;

	std::shared_ptr<SemanticType> m_currentExprType = nullptr;
	ast::Program* m_currentProgram = nullptr;

	// ==========================================
	// HELPER METHODS
	// ==========================================

	void InitBuiltins()
	{
		auto getTypeName = [&](const re::String& igniName) {
			if (m_targetConfig.primitiveMapping.contains(igniName))
			{
				return m_targetConfig.primitiveMapping.at(igniName);
			}
			return igniName;
		};

		m_context.tInt = std::make_shared<ClassType>(getTypeName("Int"));
		m_context.tDouble = std::make_shared<ClassType>(getTypeName("Double"));
		m_context.tBool = std::make_shared<ClassType>(getTypeName("Bool"));
		m_context.tString = std::make_shared<ClassType>(getTypeName("String"));
		m_context.tAny = std::make_shared<ClassType>(getTypeName("Any"));

		m_context.env.Define("Int", m_context.tInt, true);
		m_context.env.Define("Double", m_context.tDouble, true);
		m_context.env.Define("Bool", m_context.tBool, true);
		m_context.env.Define("String", m_context.tString, true);
		m_context.env.Define("Any", m_context.tAny, true);
		m_context.env.Define("Unit", m_context.tUnit, true);
		m_context.env.Define("Type", m_context.tType, true);
	}

	std::shared_ptr<SemanticType> Evaluate(const ast::Expr* expr)
	{
		if (!expr)
		{
			return m_context.tUnit;
		}

		const std::shared_ptr<SemanticType> previousType = m_currentExprType;
		m_currentExprType = m_context.tUnit;

		try
		{
			expr->Accept(*this);
		}
		catch (const SemanticBailoutException&)
		{
			m_currentExprType = m_context.tUnit;
		}

		std::shared_ptr<SemanticType> resultType = m_currentExprType;

		m_context.bindings.SetExpressionType(expr, resultType);

		m_currentExprType = previousType;

		return resultType;
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
			const ast::Statement* stmt = node->statements[i].get();
			if (const auto annoDecl = dynamic_cast<const ast::AnnotationDecl*>(stmt))
			{
				auto annoType = std::make_shared<AnnotationType>(annoDecl->name);

				if (isGlobal)
				{
					m_context.env.Define(annoDecl->name, annoType, true);
				}
				else if (currentModule)
				{
					currentModule->exports[annoDecl->name] = annoType;
				}
				continue;
			}
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
						if (!fun->typeParams.empty())
						{
							if (fun->isOverride)
							{
								IGNI_SEM_ERR(fun, "Generic methods cannot be marked 'override'");
							}

							auto tmpl = Declaration::GenericFunction(fun, currentModule ? currentModule->name : "global");
							Declaration::Overload::GenericMethod(classType, fun->name, tmpl);
							continue;
						}

						auto funType = Declaration::Method(fun, classType, m_context);
						Declaration::Overload::Method(classType, fun->name, funType);
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

	static re::String ExtractStringLiteral(const ast::Expr* expr)
	{
		if (const auto lit = dynamic_cast<const ast::LiteralExpr*>(expr))
		{
			if (lit->token.type == TokenType::StringConst)
			{
				return lit->token.lexeme.substr(1, lit->token.lexeme.length() - 2);
			}
		}
		IGNI_SEM_ERR(expr, "Expected compile-time string literal");

		return {};
	}

	bool EvaluateConstExpr(const ast::Expr* expr)
	{
		if (!expr)
		{
			return false;
		}

		if (const auto lit = dynamic_cast<const ast::LiteralExpr*>(expr))
		{ // Literals
			if (lit->token.type == TokenType::KwTrue)
			{
				return true;
			}
			if (lit->token.type == TokenType::KwFalse)
			{
				return false;
			}
			if (lit->token.type == TokenType::IntConst)
			{
				return std::stoll(lit->token.lexeme.data()) != 0;
			}
			return false;
		}

		if (const auto bin = dynamic_cast<const ast::BinaryExpr*>(expr))
		{ // Binary expressions
			using namespace re::literals;
			const auto hash = bin->op.Hashed();

			if (hash == "and"_hs)
			{
				return EvaluateConstExpr(bin->left.get()) && EvaluateConstExpr(bin->right.get());
			}
			if (hash == "or"_hs)
			{
				return EvaluateConstExpr(bin->left.get()) || EvaluateConstExpr(bin->right.get());
			}
			if (hash == "=="_hs)
			{
				return EvaluateConstExpr(bin->left.get()) == EvaluateConstExpr(bin->right.get());
			}
			if (hash == "!="_hs)
			{
				return EvaluateConstExpr(bin->left.get()) != EvaluateConstExpr(bin->right.get());
			}
		}

		if (const auto un = dynamic_cast<const ast::UnaryExpr*>(expr))
		{ // Unary expressions
			using namespace re::literals;
			if (un->op.Hashed() == "!"_hs)
			{
				return !EvaluateConstExpr(un->operand.get());
			}
		}

		if (const auto call = dynamic_cast<const ast::CallExpr*>(expr))
		{
			if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(call->callee.get()))
			{
				// Если мы вызываем метод у Type::<T>().hasAnnotation(...)
				if (const auto innerCall = dynamic_cast<const ast::CallExpr*>(memAccess->object.get()))
				{
					if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(innerCall->callee.get()))
					{
						if (id->name == "Type" && !id->typeArgs.empty())
						{
							const auto targetType = TypeResolver::Resolve(id->typeArgs[0].get(), m_context);
							const auto classType = std::dynamic_pointer_cast<ClassType>(targetType);

							if (!classType || !classType->classDecl)
							{
								return false;
							}

							// Обработка Type::<T>().hasAnnotation("Name")
							if (memAccess->member == "hasAnnotation" && call->arguments.size() == 1)
							{
								const auto annoName = ExtractStringLiteral(call->arguments[0].get());
								for (const auto& anno : classType->classDecl->annotations)
								{
									if (anno.name == annoName)
									{
										return true;
									}
								}
								return false;
							}

							// Обработка Type::<T>().hasMethodAnnotation("Method", "Name")
							if (memAccess->member == "hasMethodAnnotation" && call->arguments.size() == 2)
							{
								const auto methodName = ExtractStringLiteral(call->arguments[0].get());
								const auto annoName = ExtractStringLiteral(call->arguments[1].get());

								for (const auto& member : classType->classDecl->members)
								{
									if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
									{
										if (fun->name == methodName)
										{
											for (const auto& anno : fun->annotations)
											{
												if (anno.name == annoName)
												{
													return true;
												}
											}
										}
									}
								}
								return false;
							}
						}
					}
				}
			}
		}

		IGNI_SEM_ERR(expr, "Expression cannot be evaluated at compile time");
		return false;
	}

	void ValidateAnnotations(const ast::Decl* decl)
	{
		if (!decl)
			return;

		for (const auto& anno : decl->annotations)
		{
			const Symbol* sym = m_context.env.Resolve(anno.name);

			if (!sym)
			{
				IGNI_SEM_ERR(decl, "Undefined annotation '" + anno.name + "'");
			}

			if (!std::dynamic_pointer_cast<AnnotationType>(sym->type))
			{
				IGNI_SEM_ERR(decl, "'" + anno.name + "' is not an annotation class");
			}

			// TODO Validate argument types
		}
	}
};

} // namespace igni::sem