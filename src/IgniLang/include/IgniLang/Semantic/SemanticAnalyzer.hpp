#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem
{

class SemanticAnalyzer final : public ast::BaseAstVisitor
{
public:
	SemanticAnalyzer()
	{
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
			m_env.PushScope();

			if (const bool isGlobal = prog->packageName.Empty() || prog->packageName == "global"; !isGlobal)
			{
				if (const Symbol* modSym = m_env.Resolve(prog->packageName))
				{
					if (const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type))
					{
						for (const auto& [name, type] : modType->exports)
						{
							m_env.Define(name, type, true);
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

			m_env.PopScope();
		}

		for (std::size_t i = 0; i < m_pendingClassInstantiations.size(); ++i)
		{
			const auto* decl = m_pendingClassInstantiations[i];
			const auto classType = m_instantiatedClasses[decl->name];

			m_env.PushScope();

			if (!classType->moduleName.Empty() && classType->moduleName != "global")
			{
				if (const Symbol* modSym = m_env.Resolve(classType->moduleName))
				{
					if (const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type))
					{
						for (const auto& [name, type] : modType->exports)
							m_env.Define(name, type, true);
					}
				}
			}

			m_env.Define(decl->name, classType, true);
			for (const auto& type : classType->methods | std::views::values)
			{
				m_env.Define(type->name, type, true);
			}

			m_instantiatedNodes.erase(decl);
			for (const auto& member : decl->members)
			{
				m_instantiatedNodes.erase(member.get());
			}

			decl->Accept(*this);

			m_env.PopScope();
		}
		m_pendingClassInstantiations.clear();

		for (std::size_t i = 0; i < m_pendingFunInstantiations.size(); ++i)
		{
			const auto* decl = m_pendingFunInstantiations[i];
			auto funType = m_instantiatedFunctions[decl->name];

			m_env.PushScope();

			if (!funType->moduleName.Empty() && funType->moduleName != "global")
			{
				if (const Symbol* modSym = m_env.Resolve(funType->moduleName))
				{
					if (const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type))
					{
						for (const auto& [name, type] : modType->exports)
							m_env.Define(name, type, true);
					}
				}
			}

			m_env.Define(decl->name, funType, true);

			m_instantiatedNodes.erase(decl);
			decl->Accept(*this);

			m_env.PopScope();
		}
		m_pendingFunInstantiations.clear();
	}

	[[nodiscard]] std::shared_ptr<ClassType> GetClassType(const re::String& className) const
	{
		if (m_instantiatedClasses.contains(className))
		{
			return m_instantiatedClasses.at(className);
		}

		if (m_allClassTypes.contains(className))
		{
			return m_allClassTypes.at(className);
		}

		return nullptr;
	}

	[[nodiscard]] std::unordered_set<re::String> GetGlobalNames() const
	{
		auto globals = m_env.GetGlobalNames();
		globals.insert(m_allFunctionNames.begin(), m_allFunctionNames.end());

		return globals;
	}

	[[nodiscard]] const std::unordered_map<re::String, re::String>& GetImportAliases() const
	{
		return m_importAliases;
	}

	[[nodiscard]] const std::unordered_set<re::String>& GetExternalFunctions() const
	{
		return m_externalFunctions;
	}

	// ==========================================
	// EXPRESSIONS
	// ==========================================
	void Visit(const ast::LiteralExpr* node) override
	{
		// clang-format off
		switch (node->token.type)
		{
		case TokenType::IntConst:    m_currentExprType = m_tInt; break;
		case TokenType::FloatConst:  m_currentExprType = m_tDouble; break;
		case TokenType::StringConst: m_currentExprType = m_tString; break;
		case TokenType::KwTrue:
		case TokenType::KwFalse:     m_currentExprType = m_tBool; break;
		default:                     m_currentExprType = m_tUnit; break;
		}
		// clang-format on
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		if (node->name == "super")
		{
			if (!m_currentClassType || !m_currentClassType->baseClass)
			{
				throw std::runtime_error("Semantic Error: Cannot use 'super' outside of a derived class");
			}
			m_currentExprType = m_currentClassType->baseClass;

			return;
		}

		const Symbol* sym = m_env.Resolve(node->name);
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
			m_currentExprType = ResolveExport(modType, node->member);
		}
		else if (const auto classType = std::dynamic_pointer_cast<ClassType>(leftType))
		{
			m_currentExprType = ResolveFieldOrMethod(classType, node->member);
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

		std::shared_ptr<SemanticType> mathType;

		const bool isNumeric = (leftType == m_tInt || leftType == m_tDouble)
			&& (rightType == m_tInt || rightType == m_tDouble);
		if (isNumeric)
		{ // Type promotion for numeric types
			mathType = (leftType == m_tDouble || rightType == m_tDouble) ? m_tDouble : m_tInt;
		}
		else
		{ // lhs and rhs type comparison
			ExpectAssignable(rightType, leftType, "binary expression");
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
			m_currentExprType = m_tBool;
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

		ExpectAssignable(indexType, m_tInt, "array index");

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
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{ // Instantiating generic class member or function call
			if (id->name == "super")
			{
				isSuperCall = true;
			}

			if (const Symbol* sym = m_env.Resolve(id->name))
			{
				if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(sym->type))
				{
					if (id->typeArgs.empty())
					{
						throw std::runtime_error("Semantic Error: Generic class requires type arguments");
					}

					std::vector<std::shared_ptr<SemanticType>> concreteArgs;
					for (const auto& arg : id->typeArgs)
					{
						concreteArgs.push_back(ResolveAstType(arg.get()));
					}

					const auto concreteClass = InstantiateClass(classTmpl, concreteArgs);

					const auto mutableNode = const_cast<ast::CallExpr*>(node);
					mutableNode->isConstructorCall = true;

					if (!classTmpl->astNode->isExternal)
					{ // Mangle name for script-defined classes
						const_cast<ast::IdentifierExpr*>(id)->name = concreteClass->name;
					}
					else
					{ // Keep original name for external
						const_cast<ast::IdentifierExpr*>(id)->name = classTmpl->name;
					}
					if (const auto it = concreteClass->methods.find(concreteClass->name); it != concreteClass->methods.end())
					{
						const auto ctorType = std::dynamic_pointer_cast<FunctionType>(it->second);
						ValidateCallArguments(node, ctorType, true);
						if (!ctorType->isExternal)
						{
							mutableNode->staticMethodTarget = ctorType->name;
						}
					}
					m_currentExprType = concreteClass;

					return;
				}
				if (const auto funTmpl = std::dynamic_pointer_cast<GenericFunctionTemplate>(sym->type))
				{
					std::vector<std::shared_ptr<SemanticType>> concreteArgs;

					if (id->typeArgs.empty())
					{ // Recursive type inference
						concreteArgs.resize(funTmpl->typeParams.size(), nullptr);
						for (std::size_t i = 0; i < node->arguments.size() && i < funTmpl->astNode->parameters.size(); ++i)
						{
							const auto argType = Evaluate(node->arguments[i].get());
							const auto paramTypeAst = funTmpl->astNode->parameters[i].type.get();

							InferTypeArguments(argType, paramTypeAst, funTmpl->typeParams, concreteArgs);
						}

						for (std::size_t k = 0; k < concreteArgs.size(); ++k)
						{
							if (!concreteArgs[k])
							{
								throw std::runtime_error("Semantic Error: Could not infer type argument for '" + funTmpl->typeParams[k].name + "' in generic function '" + funTmpl->name + "'. Use explicit turbofish syntax.");
							}
						}
					}
					else
					{ // Manually named template arguments
						if (id->typeArgs.size() != funTmpl->typeParams.size())
						{
							throw std::runtime_error("Semantic Error: Generic function requires exactly " + std::to_string(funTmpl->typeParams.size()) + " type arguments");
						}

						for (const auto& arg : id->typeArgs)
						{
							concreteArgs.push_back(ResolveAstType(arg.get()));
						}
					}

					const auto concreteFun = InstantiateFunction(funTmpl, concreteArgs);

					if (!funTmpl->astNode->isExternal)
					{ // Mangle name for script-defined functions
						const_cast<ast::IdentifierExpr*>(id)->name = concreteFun->name;
					}
					else
					{ // Keep original name for external
						const_cast<ast::IdentifierExpr*>(id)->name = funTmpl->name;
					}
					ValidateCallArguments(node, concreteFun, false);

					if (concreteFun->returnType == nullptr)
					{
						throw std::runtime_error("Semantic Error: Cannot infer return type for forward call of '" + concreteFun->name + "'.");
					}

					m_currentExprType = concreteFun->returnType;
					return;
				}
			}
		}

		const auto calleeType = Evaluate(node->callee.get());

		if (const auto classType = std::dynamic_pointer_cast<ClassType>(calleeType))
		{
			const auto mutableNode = const_cast<ast::CallExpr*>(node);
			mutableNode->isConstructorCall = true;
			mutableNode->isSuperCall = isSuperCall;

			if (const auto it = classType->methods.find(classType->name); it != classType->methods.end())
			{
				const auto ctorType = std::dynamic_pointer_cast<FunctionType>(it->second);

				ValidateCallArguments(node, ctorType, true);

				if (!ctorType->isExternal)
				{
					mutableNode->staticMethodTarget = ctorType->name;
				}
			}

			m_currentExprType = classType;
			return;
		}

		const auto funType = std::dynamic_pointer_cast<FunctionType>(calleeType);
		if (!funType)
		{
			throw std::runtime_error("Semantic Error: Attempt to call a non-callable type");
		}

		bool isMethodCall = false;
		if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{ // Static class methods dispatching
			const auto objectType = Evaluate(memAccess->object.get());
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(objectType))
			{
				isMethodCall = true;
				const auto mutableNode = const_cast<ast::CallExpr*>(node);
				if (!funType->isExternal)
				{
					mutableNode->staticMethodTarget = classType->name + "_" + memAccess->member;
				}
			}
		}

		ValidateCallArguments(node, funType, isMethodCall);

		if (funType->returnType == nullptr)
		{
			throw std::runtime_error("Semantic Error: Cannot infer return type for forward call of '" + funType->name + "'. Please specify the return type explicitly or move the declaration above the call.");
		}

		m_currentExprType = funType->returnType;
	}

	// ==========================================
	// INSTRUCTIONS
	// ==========================================
	void Visit(const ast::VarDecl* node) override
	{
		HandleVariableDeclaration(node->name, node->initializer.get(), node->type.get(), false);
	}

	void Visit(const ast::ValDecl* node) override
	{
		HandleVariableDeclaration(node->name, node->initializer.get(), node->type.get(), true);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		Evaluate(node->expr.get());
	}

	void Visit(const ast::ConstructorDecl* node) override
	{
		if (m_instantiatedNodes.contains(node))
		{
			return;
		}

		const Symbol* sym = m_env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

		m_env.PushScope();

		// Parameters including 'this'
		for (std::size_t i = 0; i < node->parameters.size(); ++i)
		{
			std::shared_ptr<SemanticType> paramType = funType->paramTypes[i];
			if (node->isVararg && i == node->parameters.size() - 1)
			{
				paramType = GetVarargArrayType(paramType);
			}
			m_env.Define(node->parameters[i].name, paramType, false);
		}

		const auto previousReturnType = m_currentReturnType;
		const auto previousFunctionType = m_currentFunctionType;

		m_currentReturnType = funType->returnType; // Unit
		m_currentFunctionType = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_currentReturnType = previousReturnType;
		m_currentFunctionType = previousFunctionType;
		m_env.PopScope();
	}

	void Visit(const ast::DestructorDecl* node) override
	{
		if (m_instantiatedNodes.contains(node))
		{
			return;
		}

		const Symbol* sym = m_env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

		m_env.PushScope();

		m_env.Define("this", funType->paramTypes[0], false);

		const auto previousReturnType = m_currentReturnType;
		const auto previousFunctionType = m_currentFunctionType;

		m_currentReturnType = funType->returnType; // Unit
		m_currentFunctionType = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_currentReturnType = previousReturnType;
		m_currentFunctionType = previousFunctionType;
		m_env.PopScope();
	}

	void Visit(const ast::AssignExpr* node) override
	{
		const auto targetType = Evaluate(node->target.get());
		const auto valueType = Evaluate(node->value.get());

		// Mutability check
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{ // val ident
			if (const Symbol* sym = m_env.Resolve(id->name); sym && sym->isReadOnly)
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
						if (m_currentFunctionType)
						{ // allow val mutations in constructor
							const re::String expectedCtorName = classType->name + "_" + classType->name;
							if (m_currentFunctionType->name == expectedCtorName)
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

		ExpectAssignable(valueType, targetType, "assignment");
		m_currentExprType = valueType;
	}

	void Visit(const ast::IfStmt* node) override
	{
		const auto condType = Evaluate(node->condition.get());
		ExpectAssignable(condType, m_tBool, "if condition");

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
		ExpectAssignable(condType, m_tBool, "while condition");

		if (node->body)
		{
			node->body->Accept(*this);
		}
	}

	void Visit(const ast::ForStmt* node) override
	{
		m_env.PushScope();

		Evaluate(node->startExpr.get());
		Evaluate(node->endExpr.get());

		m_env.Define(node->iteratorName, m_tInt, false);

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_env.PopScope();
	}

	void Visit(const ast::Block* node) override
	{
		m_env.PushScope();
		for (const auto& s : node->statements)
		{
			if (s)
			{
				s->Accept(*this);
			}
		}
		m_env.PopScope();
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		const auto retType = Evaluate(node->expr.get());

		if (m_currentFunctionType && m_currentFunctionType->returnType == nullptr)
		{ // Resolving a type of return expression
			m_currentFunctionType->returnType = retType;
			m_currentReturnType = retType;
		}
		else if (m_currentReturnType)
		{
			ExpectAssignable(retType, m_currentReturnType, "return statement");
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (m_instantiatedNodes.contains(node))
		{
			return;
		}

		if (!node->typeParams.empty())
		{ // Ignore generic function declarations
			return;
		}

		std::shared_ptr<FunctionType> funType;

		if (m_currentFunctionType != nullptr)
		{ // Local function
			funType = std::make_shared<FunctionType>(node->name);
			funType->returnType = ResolveAstType(node->returnType.get());

			if (funType->returnType == nullptr && !node->isExprBody)
			{
				funType->returnType = m_tUnit;
			}

			funType->isVararg = node->isVararg;

			for (const auto& [_, type] : node->parameters)
			{
				funType->paramTypes.emplace_back(ResolveAstType(type.get()));
			}
			m_env.Define(node->name, funType, true);
		}
		else
		{ // Global function
			const Symbol* sym = m_env.Resolve(node->name);
			funType = std::dynamic_pointer_cast<FunctionType>(sym->type);
		}

		m_env.PushScope();
		for (std::size_t i = 0; i < node->parameters.size(); ++i)
		{
			std::shared_ptr<SemanticType> paramType = funType->paramTypes[i];
			if (node->isVararg && i == node->parameters.size() - 1)
			{
				paramType = GetVarargArrayType(paramType);
			}
			m_env.Define(node->parameters[i].name, paramType, false);
		}

		const auto previousReturnType = m_currentReturnType;
		const auto previousFunctionType = m_currentFunctionType;

		m_currentReturnType = funType->returnType;
		m_currentFunctionType = funType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		if (m_currentFunctionType->returnType == nullptr)
		{ // No return in function -> fallback to Unit
			m_currentFunctionType->returnType = m_tUnit;
		}

		m_currentReturnType = previousReturnType;
		m_currentFunctionType = previousFunctionType;
		m_env.PopScope();
	}

	void Visit(const ast::ClassDecl* node) override
	{
		if (m_instantiatedNodes.contains(node))
		{
			return;
		}

		if (!node->typeParams.empty())
		{ // Ignore generic classes declarations
			return;
		}

		std::shared_ptr<ClassType> classType;
		if (const Symbol* sym = m_env.Resolve(node->name))
		{
			classType = std::dynamic_pointer_cast<ClassType>(sym->type);
		}
		else if (m_instantiatedClasses.contains(node->name))
		{
			classType = m_instantiatedClasses.at(node->name);
		}
		else
		{
			throw std::runtime_error("Semantic Error: Class '" + node->name + "' not found during Visit");
		}

		const auto previousClass = m_currentClassType;
		m_currentClassType = classType;

		if (node->baseClass)
		{
			const auto baseType = ResolveAstType(node->baseClass->type.get());
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
			if (const Symbol* anySym = m_env.Resolve("Any"))
			{
				classType->baseClass = std::dynamic_pointer_cast<ClassType>(anySym->type);
			}
		}

		for (const auto& member : node->members)
		{ // Register member values
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{ // Member var
				auto fieldType = ResolveAstType(varDecl->type.get());
				if (varDecl->initializer)
				{
					const auto initType = Evaluate(varDecl->initializer.get());
					if (!fieldType || fieldType == m_tUnit)
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
				auto fieldType = ResolveAstType(valDecl->type.get());
				if (valDecl->initializer)
				{
					const auto initType = Evaluate(valDecl->initializer.get());
					if (!fieldType || fieldType == m_tUnit)
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

		m_currentClassType = previousClass;
	}

	void Visit(const ast::ImportDecl* node) override
	{
		ProcessImport(node);
	}

private:
	Environment m_env;
	std::shared_ptr<SemanticType> m_currentExprType = nullptr;
	std::shared_ptr<SemanticType> m_currentReturnType = nullptr;
	std::shared_ptr<FunctionType> m_currentFunctionType = nullptr;
	std::shared_ptr<ModuleType> m_currentPackage = nullptr;
	std::shared_ptr<ClassType> m_currentClassType = nullptr;
	ast::Program* m_currentProgram = nullptr;

	std::unordered_map<re::String, std::shared_ptr<ClassType>> m_instantiatedClasses;
	std::vector<ast::ClassDecl*> m_pendingClassInstantiations;

	std::unordered_map<re::String, std::shared_ptr<FunctionType>> m_instantiatedFunctions;
	std::vector<ast::FunDecl*> m_pendingFunInstantiations;

	std::unordered_set<const ast::Node*> m_instantiatedNodes;

	std::unordered_map<re::String, std::shared_ptr<ClassType>> m_allClassTypes;

	std::unordered_map<re::String, re::String> m_importAliases;
	std::unordered_set<re::String> m_externalFunctions;
	std::unordered_set<re::String> m_allFunctionNames;

	// --- Type Definitions ---
	std::shared_ptr<PrimitiveType> m_tInt = std::make_shared<PrimitiveType>("Int");
	std::shared_ptr<PrimitiveType> m_tDouble = std::make_shared<PrimitiveType>("Double");
	std::shared_ptr<PrimitiveType> m_tBool = std::make_shared<PrimitiveType>("Bool");
	std::shared_ptr<ClassType> m_tString = std::make_shared<ClassType>("String");
	std::shared_ptr<PrimitiveType> m_tUnit = std::make_shared<PrimitiveType>("Unit");
	std::shared_ptr<PrimitiveType> m_tAny = std::make_shared<PrimitiveType>("Any");

	// ==========================================
	// HELPER METHODS
	// ==========================================

	void InitBuiltins()
	{
		m_env.Define("Int", m_tInt, true);
		m_env.Define("Double", m_tDouble, true);
		m_env.Define("Bool", m_tBool, true);
		m_env.Define("String", m_tString, true);
		m_env.Define("Unit", m_tUnit, true);
		m_env.Define("Any", m_tAny, true);
	}

	std::shared_ptr<SemanticType> Evaluate(const ast::Expr* expr)
	{
		if (!expr)
		{
			return m_tUnit;
		}

		m_currentExprType = m_tUnit; // Reset state
		expr->Accept(*this);

		return m_currentExprType;
	}

	void InferTypeArguments(const std::shared_ptr<SemanticType>& argType,
		const ast::TypeNode* paramTypeAst,
		const std::vector<ast::GenericTypeParam>& typeParams,
		std::vector<std::shared_ptr<SemanticType>>& outConcreteArgs)
	{
		if (!argType || !paramTypeAst)
			return;

		if (const auto simpleAst = dynamic_cast<const ast::SimpleTypeNode*>(paramTypeAst))
		{
			for (std::size_t k = 0; k < typeParams.size(); ++k)
			{ // Basic case: T is T
				if (typeParams[k].name == simpleAst->name)
				{
					if (!outConcreteArgs[k])
					{
						outConcreteArgs[k] = argType;
					}
					return;
				}
			}

			// Рекурсивный случай 1: параметр — это дженерик-класс (например, Array<T>)
			if (!simpleAst->typeArgs.empty())
			{ // Recursive case 1: T is generic argument of Class<T>
				if (const auto classType = std::dynamic_pointer_cast<ClassType>(argType))
				{
					for (std::size_t i = 0; i < simpleAst->typeArgs.size() && i < classType->typeArguments.size(); ++i)
					{
						InferTypeArguments(classType->typeArguments[i], simpleAst->typeArgs[i].get(), typeParams, outConcreteArgs);
					}
				}
			}
		}
		else if (const auto funAst = dynamic_cast<const ast::FunctionTypeNode*>(paramTypeAst))
		{ // Recursive case 2: T is generic function argument like callback: (T) -> Unit
			if (const auto funType = std::dynamic_pointer_cast<FunctionType>(argType))
			{
				for (std::size_t i = 0; i < funAst->paramTypes.size() && i < funType->paramTypes.size(); ++i)
				{
					InferTypeArguments(funType->paramTypes[i], funAst->paramTypes[i].get(), typeParams, outConcreteArgs);
				}
				InferTypeArguments(funType->returnType, funAst->returnType.get(), typeParams, outConcreteArgs);
			}
		}
	}

	std::shared_ptr<SemanticType> GetVarargArrayType(const std::shared_ptr<SemanticType>& elementType)
	{
		if (const Symbol* sym = m_env.Resolve("Array"))
		{ // Instantiating Array class
			if (const auto tmpl = std::dynamic_pointer_cast<GenericClassTemplate>(sym->type))
			{
				return InstantiateClass(tmpl, { elementType });
			}

			return std::dynamic_pointer_cast<SemanticType>(sym->type);
		}

		return m_tUnit; // Fallback in case stdlib is not loaded
	}

	void RegisterProgramDeclarations(const ast::Program* node)
	{
		const bool isGlobal = node->packageName.Empty() || node->packageName == "global";
		std::shared_ptr<ModuleType> currentModule = nullptr;

		if (!isGlobal)
		{
			if (const Symbol* sym = m_env.Resolve(node->packageName))
			{
				currentModule = std::dynamic_pointer_cast<ModuleType>(sym->type);
			}

			if (!currentModule)
			{
				currentModule = std::make_shared<ModuleType>(node->packageName);
				m_env.Define(node->packageName, currentModule, true);
			}
		}

		m_currentPackage = currentModule;

		for (const auto& stmt : node->statements)
		{
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				if (!classDecl->typeParams.empty())
				{ // Generic non-instantiated class
					auto tmpl = std::make_shared<GenericClassTemplate>(classDecl->name);
					tmpl->astNode = classDecl;
					tmpl->typeParams = classDecl->typeParams;
					tmpl->moduleName = currentModule ? currentModule->name : "global";

					if (isGlobal)
					{
						m_env.Define(classDecl->name, tmpl, true);
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
					if (const Symbol* existingSym = m_env.Resolve(classDecl->name))
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

				m_allClassTypes[classDecl->name] = classType;

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
					m_env.Define(classDecl->name, classType, true);
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

						ast::FunDecl::Parameter thisParam;
						thisParam.name = "this";
						auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
						thisTypeNode->name = classDecl->name;
						thisParam.type = std::move(thisTypeNode);
						mutableFun->parameters.insert(mutableFun->parameters.begin(), std::move(thisParam));

						mutableFun->name = classDecl->name + "_" + originalName;
						m_allFunctionNames.insert(mutableFun->name);

						auto funType = std::make_shared<FunctionType>(mutableFun->name);
						funType->returnType = ResolveAstType(mutableFun->returnType.get());
						if (funType->returnType == nullptr && !mutableFun->isExprBody)
						{
							funType->returnType = m_tUnit;
						}
						funType->isVararg = mutableFun->isVararg;

						for (const auto& [_, type] : mutableFun->parameters)
						{
							funType->paramTypes.emplace_back(ResolveAstType(type.get()));
						}

						funType->visibility = mutableFun->visibility;
						funType->moduleName = currentModule ? currentModule->name : "global";
						funType->isExternal = fun->isExternal || classDecl->isExternal;

						if (isGlobal)
						{
							m_env.Define(mutableFun->name, funType, true);
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
						m_allFunctionNames.insert(mutableCtor->name);

						auto funType = std::make_shared<FunctionType>(mutableCtor->name);
						funType->returnType = m_tUnit;
						funType->isVararg = mutableCtor->isVararg;
						funType->isExternal = classDecl->isExternal;

						for (const auto& [_, type] : mutableCtor->parameters)
						{
							funType->paramTypes.emplace_back(ResolveAstType(type.get()));
						}

						funType->visibility = mutableCtor->visibility;

						if (isGlobal)
						{
							m_env.Define(mutableCtor->name, funType, true);
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
						m_allFunctionNames.insert(mutableDtor->name);

						auto funType = std::make_shared<FunctionType>(mutableDtor->name);
						funType->returnType = m_tUnit;

						auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
						thisTypeNode->name = classDecl->name;
						funType->paramTypes.emplace_back(ResolveAstType(thisTypeNode.get()));

						funType->visibility = mutableDtor->visibility;
						funType->isExternal = classDecl->isExternal;

						if (isGlobal)
						{
							m_env.Define(mutableDtor->name, funType, true);
						}
						else
						{
							currentModule->exports[mutableDtor->name] = funType;
						}

						classType->methods["~" + classDecl->name] = funType;
					}
				}
			}

			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				if (!fun->typeParams.empty())
				{ // Generic non-instantiated function
					auto tmpl = std::make_shared<GenericFunctionTemplate>(fun->name);
					tmpl->astNode = fun;
					tmpl->typeParams = fun->typeParams;
					tmpl->moduleName = currentModule ? currentModule->name : "global";

					if (isGlobal)
					{
						m_env.Define(fun->name, tmpl, true);
					}
					else
					{
						currentModule->exports[fun->name] = tmpl;
					}

					if (fun->isExternal)
					{
						m_externalFunctions.insert(fun->name);
					}

					continue;
				}

				// Non-generic or instantiated function
				m_allFunctionNames.insert(fun->name);

				auto funType = std::make_shared<FunctionType>(fun->name);
				funType->returnType = ResolveAstType(fun->returnType.get());

				if (funType->returnType == nullptr && !fun->isExprBody)
				{
					funType->returnType = m_tUnit;
				}

				funType->isVararg = fun->isVararg;

				for (const auto& [_, type] : fun->parameters)
				{
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				}

				funType->visibility = fun->visibility;
				funType->moduleName = currentModule ? currentModule->name : "global";

				if (isGlobal)
				{
					m_env.Define(funType->name, funType, true);
				}
				else
				{
					currentModule->exports[funType->name] = funType;
				}

				if (fun->isExternal)
				{
					m_externalFunctions.insert(fun->name);
				}
			}
		}

		m_currentPackage = nullptr;
	}

	static void ExpectAssignable(const std::shared_ptr<SemanticType>& actual, const std::shared_ptr<SemanticType>& expected, const std::string& context)
	{
		if (!actual->IsAssignableTo(expected.get()))
		{
			throw std::runtime_error("Semantic Error: Type mismatch in " + context + ". Expected " + expected->name + ", got " + actual->name);
		}
	}

	void HandleVariableDeclaration(const re::String& name, const ast::Expr* initializer, const ast::TypeNode* explicitTypeNode, bool isReadOnly)
	{
		auto varType = Evaluate(initializer);

		if (explicitTypeNode)
		{
			const auto declaredType = ResolveAstType(explicitTypeNode);
			if (initializer)
			{
				ExpectAssignable(varType, declaredType, "variable initialization");
			}
			varType = declaredType;
		}

		if (varType == m_tUnit && !explicitTypeNode)
		{
			throw std::runtime_error("Semantic Error: Cannot infer type for variable '" + name + "'");
		}

		m_env.Define(name, varType, isReadOnly);
	}

	static std::shared_ptr<SemanticType> ResolveExport(const std::shared_ptr<ModuleType>& modType, const re::String& member)
	{
		if (const auto it = modType->exports.find(member); it != modType->exports.end())
		{
			return it->second;
		}
		throw std::runtime_error("Semantic Error: Module '" + modType->name + "' has no export named '" + member + "'");
	}

	std::shared_ptr<SemanticType> ResolveFieldOrMethod(const std::shared_ptr<ClassType>& classType, const re::String& member)
	{
		auto vis = ast::Visibility::Public;
		std::shared_ptr<SemanticType> resolvedType = nullptr;

		if (const auto fieldIt = classType->fields.find(member); fieldIt != classType->fields.end())
		{
			resolvedType = fieldIt->second.type;
			vis = fieldIt->second.visibility;
		}
		else if (const auto methodIt = classType->methods.find(member); methodIt != classType->methods.end())
		{
			resolvedType = methodIt->second;
			vis = methodIt->second->visibility;
		}
		else
		{
			throw std::runtime_error("Semantic Error: Class '" + classType->name + "' has no field or method named '" + member + "'");
		}

		if (vis == ast::Visibility::Private)
		{
			if (!m_currentClassType || m_currentClassType->name != classType->name)
			{
				throw std::runtime_error("Semantic Error: Cannot access private member '" + member + "' of class '" + classType->name + "'");
			}
		}
		else if (vis == ast::Visibility::Internal)
		{
			const re::String currentPkg = m_currentPackage ? m_currentPackage->name : "global";
			const re::String targetPkg = classType->moduleName.Empty() ? "global" : classType->moduleName;
			if (currentPkg != targetPkg)
			{
				throw std::runtime_error("Semantic Error: Cannot access internal member '" + member + "' outside of its package '" + targetPkg + "'");
			}
		}

		return resolvedType;
	}

	void ValidateCallArguments(const ast::CallExpr* node, const std::shared_ptr<FunctionType>& funType, bool isMethodCall = false)
	{
		const std::size_t thisOffset = isMethodCall ? 1 : 0;

		if (funType->isVararg)
		{
			const std::size_t fixedParams = funType->paramTypes.size() - 1 - thisOffset;
			if (node->arguments.size() < fixedParams)
			{
				throw std::runtime_error("Semantic Error: Not enough arguments for vararg function");
			}

			const auto mutableNode = const_cast<ast::CallExpr*>(node);
			mutableNode->isVarargCall = true;
			mutableNode->varargCount = node->arguments.size() - fixedParams;
		}
		else if (node->arguments.size() + thisOffset != funType->paramTypes.size())
		{
			throw std::runtime_error("Semantic Error: Argument count mismatch");
		}

		for (std::size_t i = 0; i < node->arguments.size(); ++i)
		{
			const auto argType = Evaluate(node->arguments[i].get());

			std::shared_ptr<SemanticType> expectedType = (funType->isVararg && i + thisOffset >= funType->paramTypes.size() - 1)
				? funType->paramTypes.back()
				: funType->paramTypes[i + thisOffset];

			ExpectAssignable(argType, expectedType, "argument " + std::to_string(i + 1));
		}
	}

	void ProcessImport(const ast::ImportDecl* node)
	{
		const re::String fullPath = node->path;

		if (node->isStar)
		{ // import module.*
			const Symbol* modSym = m_env.Resolve(fullPath);
			if (!modSym)
			{
				throw std::runtime_error("Semantic Error: Unknown module '" + fullPath + "' in import");
			}

			const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type);
			if (!modType)
			{
				throw std::runtime_error("Semantic Error: '" + fullPath + "' is not a module");
			}

			for (const auto& [exportName, exportType] : modType->exports)
			{
				m_env.Define(exportName, exportType, true);
				m_importAliases[exportName] = fullPath;
			}
		}
		else
		{ // import module.ident
			const std::size_t dotPos = fullPath.Find('.');
			if (dotPos == re::String::NPos)
			{
				return;
			}

			const auto modName = fullPath.Substring(0, dotPos);
			const auto memberName = fullPath.Substring(dotPos + 1);

			const Symbol* modSym = m_env.Resolve(modName);
			if (!modSym)
			{
				throw std::runtime_error("Semantic Error: Unknown module '" + modName + "' in import");
			}

			const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type);
			if (!modType)
			{
				throw std::runtime_error("Semantic Error: '" + modName + "' is not a module");
			}

			if (const auto it = modType->exports.find(memberName); it != modType->exports.end())
			{
				m_env.Define(memberName, it->second, true);
				m_importAliases[memberName] = modName;
			}
			else
			{
				throw std::runtime_error("Semantic Error: Module '" + modName + "' has no export named '" + memberName + "'");
			}
		}
	}

	std::shared_ptr<SemanticType> ResolveAstType(const ast::TypeNode* typeNode)
	{
		if (!typeNode)
		{ // Type must be resolved via expression
			return nullptr;
		}

		if (const auto simpleType = dynamic_cast<const ast::SimpleTypeNode*>(typeNode))
		{
			using namespace re::literals;

			// clang-format off
			switch (simpleType->name.Hashed())
			{
			case "Int"_hs:    return m_tInt;
			case "Double"_hs: return m_tDouble;
			case "Bool"_hs:   return m_tBool;
			case "String"_hs: return m_tString;
			case "Unit"_hs:   return m_tUnit;
			default:          break;
			}
			// clang-format on

			if (const Symbol* sym = m_env.Resolve(simpleType->name))
			{
				if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(sym->type))
				{ // Instantiate generic class
					if (simpleType->typeArgs.empty())
					{
						throw std::runtime_error("Semantic Error: Generic class '" + simpleType->name + "' requires type arguments");
					}

					std::vector<std::shared_ptr<SemanticType>> concreteArgs;
					for (const auto& arg : simpleType->typeArgs)
					{
						concreteArgs.push_back(ResolveAstType(arg.get()));
					}

					return InstantiateClass(classTmpl, concreteArgs);
				}

				return sym->type;
			}

			if (m_currentPackage)
			{
				if (const auto it = m_currentPackage->exports.find(simpleType->name); it != m_currentPackage->exports.end())
				{
					return it->second;
				}
			}

			if (m_instantiatedClasses.contains(simpleType->name))
			{
				return m_instantiatedClasses.at(simpleType->name);
			}

			throw std::runtime_error("Semantic Error: Unknown type '" + simpleType->name + "'");
		}

		if (const auto funTypeNode = dynamic_cast<const ast::FunctionTypeNode*>(typeNode))
		{
			// TODO: Generate unique anonymous type
			auto semFunType = std::make_shared<FunctionType>("<anonymous_lambda>");

			for (const auto& pType : funTypeNode->paramTypes)
			{
				semFunType->paramTypes.push_back(ResolveAstType(pType.get()));
			}

			semFunType->returnType = ResolveAstType(funTypeNode->returnType.get());

			return semFunType;
		}

		return m_tUnit;
	}

	std::shared_ptr<ClassType> InstantiateClass(const std::shared_ptr<GenericClassTemplate>& tmpl, const std::vector<std::shared_ptr<SemanticType>>& typeArgs)
	{
		re::String uniqueName = tmpl->name;
		for (const auto& arg : typeArgs)
		{
			uniqueName = uniqueName + "_" + arg->name;
		}

		if (m_instantiatedClasses.contains(uniqueName))
		{
			return m_instantiatedClasses[uniqueName];
		}

		ast::TypeEnv typeEnv;
		std::vector<std::unique_ptr<ast::SimpleTypeNode>> tempNodes;

		for (std::size_t i = 0; i < typeArgs.size(); ++i)
		{
			if (tmpl->typeParams[i].boundType)
			{
				auto boundSemType = ResolveAstType(tmpl->typeParams[i].boundType.get());
				ExpectAssignable(typeArgs[i], boundSemType, "type parameter bound");
			}
			auto tempNode = std::make_unique<ast::SimpleTypeNode>();
			tempNode->name = typeArgs[i]->name;
			typeEnv[tmpl->typeParams[i].name] = tempNode.get();
			tempNodes.push_back(std::move(tempNode));
		}

		auto clonedDecl = tmpl->astNode->CloneDecl(&typeEnv);
		auto realClassDecl = dynamic_cast<ast::ClassDecl*>(clonedDecl.get());
		realClassDecl->name = uniqueName;
		realClassDecl->typeParams.clear();

		bool hasConstructor = false;
		for (const auto& member : realClassDecl->members)
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
			defaultCtor->name = uniqueName;
			defaultCtor->body = std::make_unique<ast::Block>();
			realClassDecl->members.push_back(std::move(defaultCtor));
		}

		auto classType = std::make_shared<ClassType>(uniqueName);
		classType->moduleName = tmpl->moduleName;
		classType->typeArguments = typeArgs;
		m_instantiatedClasses[uniqueName] = classType;

		m_env.Define(uniqueName, classType, true);
		for (const auto& member : realClassDecl->members)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{
				auto fieldType = ResolveAstType(varDecl->type.get());
				if (varDecl->initializer)
				{
					const auto initType = Evaluate(varDecl->initializer.get());
					if (!fieldType || fieldType == m_tUnit)
					{
						fieldType = initType;
					}
				}
				else if (!varDecl->isExternal && !realClassDecl->isExternal && !fieldType)
				{
					throw std::runtime_error("Semantic Error: Property '" + varDecl->name + "' must have an initializer or explicit type");
				}

				classType->fields[varDecl->name] = { fieldType, false, varDecl->visibility };
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{
				auto fieldType = ResolveAstType(valDecl->type.get());
				if (valDecl->initializer)
				{
					const auto initType = Evaluate(valDecl->initializer.get());
					if (!fieldType || fieldType == m_tUnit)
					{
						fieldType = initType;
					}
				}
				else if (!valDecl->isExternal && !realClassDecl->isExternal && !fieldType)
				{
					throw std::runtime_error("Semantic Error: Property '" + valDecl->name + "' must have an initializer or explicit type");
				}

				classType->fields[valDecl->name] = { fieldType, false, valDecl->visibility };
			}
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
			{
				if (realClassDecl->isExternal && (!fun->isExternal || fun->body || fun->isExprBody))
				{
					throw std::runtime_error("Semantic Error: Method '" + fun->name + "' in external class '" + realClassDecl->name + "' must be marked 'external' and cannot have a body.");
				}

				const auto mutableFun = const_cast<ast::FunDecl*>(fun);
				const re::String originalName = mutableFun->name;

				ast::FunDecl::Parameter thisParam;
				thisParam.name = "this";
				auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
				thisTypeNode->name = uniqueName;
				thisParam.type = std::move(thisTypeNode);
				mutableFun->parameters.insert(mutableFun->parameters.begin(), std::move(thisParam));

				mutableFun->name = uniqueName + "_" + originalName;
				m_allFunctionNames.insert(mutableFun->name);

				auto funType = std::make_shared<FunctionType>(mutableFun->name);
				funType->returnType = ResolveAstType(mutableFun->returnType.get());
				if (funType->returnType == nullptr && !mutableFun->isExprBody)
				{
					funType->returnType = m_tUnit;
				}
				funType->isVararg = mutableFun->isVararg;
				for (const auto& [_, type] : mutableFun->parameters)
				{
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				}

				if (funType->returnType == nullptr && mutableFun->isExprBody)
				{ // On-demand type evaluation for expression functions
					m_env.PushScope();

					for (std::size_t j = 0; j < mutableFun->parameters.size(); ++j)
					{
						m_env.Define(mutableFun->parameters[j].name, funType->paramTypes[j], false);
					}

					const auto prevFun = m_currentFunctionType;
					m_currentFunctionType = funType;

					if (mutableFun->body && !mutableFun->body->statements.empty())
					{
						if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(mutableFun->body->statements[0].get()))
						{
							funType->returnType = Evaluate(retStmt->expr.get());
						}
					}

					m_currentFunctionType = prevFun;
					m_env.PopScope();
				}

				funType->visibility = mutableFun->visibility;
				funType->isExternal = fun->isExternal || realClassDecl->isExternal;
				if (funType->isExternal)
				{
					m_externalFunctions.insert(funType->name);
				}

				bool existsInBase = classType->baseClass && classType->baseClass->methods.contains(originalName);

				if (mutableFun->isOverride && !existsInBase)
				{
					throw std::runtime_error("Semantic Error: Method '" + originalName + "' is marked 'override' but no matching method found in base class '" + classType->baseClass->name + "'");
				}
				if (!mutableFun->isOverride && existsInBase)
				{
					throw std::runtime_error("Semantic Error: Method '" + originalName + "' hides base class method. Add the 'override' modifier.");
				}

				classType->methods[originalName] = funType;
			}
			else if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
			{
				const auto mutableCtor = const_cast<ast::ConstructorDecl*>(ctor);

				ast::FunDecl::Parameter thisParam;
				thisParam.name = "this";
				auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
				thisTypeNode->name = uniqueName;
				thisParam.type = std::move(thisTypeNode);
				mutableCtor->parameters.insert(mutableCtor->parameters.begin(), std::move(thisParam));

				mutableCtor->name = uniqueName + "_" + uniqueName;
				m_allFunctionNames.insert(mutableCtor->name);

				auto funType = std::make_shared<FunctionType>(mutableCtor->name);
				funType->returnType = m_tUnit;
				funType->isVararg = mutableCtor->isVararg;
				for (const auto& [_, type] : mutableCtor->parameters)
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				funType->isExternal = realClassDecl->isExternal;

				classType->methods[uniqueName] = funType;
			}
		}

		m_pendingClassInstantiations.push_back(realClassDecl);
		m_instantiatedNodes.insert(realClassDecl);
		for (const auto& member : realClassDecl->members)
		{
			m_instantiatedNodes.insert(member.get());
		}

		m_currentProgram->statements.push_back(std::move(clonedDecl));

		return classType;
	}

	void RegisterClassMembers(const ast::ClassDecl* classDecl, const std::shared_ptr<ClassType>& classType, const std::shared_ptr<ModuleType>& currentModule)
	{
		const bool isGlobal = currentModule == nullptr;

		for (const auto& member : classDecl->members)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
			{
				const auto mutableFun = const_cast<ast::FunDecl*>(fun);
				const re::String originalName = mutableFun->name;

				ast::FunDecl::Parameter thisParam;
				thisParam.name = "this";
				auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
				thisTypeNode->name = classDecl->name;
				thisParam.type = std::move(thisTypeNode);
				mutableFun->parameters.insert(mutableFun->parameters.begin(), std::move(thisParam));

				mutableFun->name = classDecl->name + "_" + originalName;
				m_allFunctionNames.insert(mutableFun->name);

				auto funType = std::make_shared<FunctionType>(mutableFun->name);
				funType->returnType = ResolveAstType(mutableFun->returnType.get());
				if (funType->returnType == nullptr && !mutableFun->isExprBody)
				{
					funType->returnType = m_tUnit;
				}
				funType->isVararg = mutableFun->isVararg;

				for (const auto& [_, type] : mutableFun->parameters)
				{
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				}

				funType->visibility = mutableFun->visibility;
				funType->moduleName = currentModule ? currentModule->name : "global";
				funType->isExternal = fun->isExternal || classDecl->isExternal;

				if (isGlobal)
				{
					m_env.Define(mutableFun->name, funType, true);
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
				m_allFunctionNames.insert(mutableCtor->name);

				auto funType = std::make_shared<FunctionType>(mutableCtor->name);
				funType->returnType = m_tUnit;
				funType->isVararg = mutableCtor->isVararg;
				funType->isExternal = classDecl->isExternal;

				for (const auto& [_, type] : mutableCtor->parameters)
				{
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				}

				funType->visibility = mutableCtor->visibility;

				if (isGlobal)
				{
					m_env.Define(mutableCtor->name, funType, true);
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
				m_allFunctionNames.insert(mutableDtor->name);

				auto funType = std::make_shared<FunctionType>(mutableDtor->name);
				funType->returnType = m_tUnit;

				auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
				thisTypeNode->name = classDecl->name;
				funType->paramTypes.emplace_back(ResolveAstType(thisTypeNode.get()));

				funType->visibility = mutableDtor->visibility;
				funType->isExternal = classDecl->isExternal;

				if (isGlobal)
				{
					m_env.Define(mutableDtor->name, funType, true);
				}
				else
				{
					currentModule->exports[mutableDtor->name] = funType;
				}

				classType->methods["~" + classDecl->name] = funType;
			}
		}
	}

	std::shared_ptr<FunctionType> InstantiateFunction(const std::shared_ptr<GenericFunctionTemplate>& tmpl, const std::vector<std::shared_ptr<SemanticType>>& typeArgs)
	{
		re::String uniqueName = tmpl->name;
		for (const auto& arg : typeArgs)
			uniqueName = uniqueName + "_" + arg->name;

		if (m_instantiatedFunctions.contains(uniqueName))
			return m_instantiatedFunctions[uniqueName];

		ast::TypeEnv typeEnv;
		std::vector<std::unique_ptr<ast::SimpleTypeNode>> tempNodes;

		for (std::size_t i = 0; i < typeArgs.size(); ++i)
		{
			if (tmpl->typeParams[i].boundType)
			{
				auto boundSemType = ResolveAstType(tmpl->typeParams[i].boundType.get());
				ExpectAssignable(typeArgs[i], boundSemType, "type parameter bound");
			}
			auto tempNode = std::make_unique<ast::SimpleTypeNode>();
			tempNode->name = typeArgs[i]->name;
			typeEnv[tmpl->typeParams[i].name] = tempNode.get();
			tempNodes.push_back(std::move(tempNode));
		}

		// Клонируем AST с подстановкой типов
		auto clonedDecl = tmpl->astNode->CloneDecl(&typeEnv);
		auto realFunDecl = dynamic_cast<ast::FunDecl*>(clonedDecl.get());
		realFunDecl->name = uniqueName;
		realFunDecl->typeParams.clear();

		auto funType = std::make_shared<FunctionType>(uniqueName);
		funType->moduleName = tmpl->moduleName;
		funType->isVararg = realFunDecl->isVararg;
		funType->visibility = realFunDecl->visibility;
		funType->isExternal = realFunDecl->isExternal;

		for (const auto& p : realFunDecl->parameters)
		{
			funType->paramTypes.push_back(ResolveAstType(p.type.get()));
		}
		funType->returnType = ResolveAstType(realFunDecl->returnType.get());

		if (funType->returnType == nullptr && realFunDecl->isExprBody)
		{ // On-demand type evaluation
			m_env.PushScope();
			for (std::size_t j = 0; j < realFunDecl->parameters.size(); ++j)
			{
				m_env.Define(realFunDecl->parameters[j].name, funType->paramTypes[j], false);
			}

			const auto prevFun = m_currentFunctionType;
			m_currentFunctionType = funType;

			if (realFunDecl->body && !realFunDecl->body->statements.empty())
			{
				if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(realFunDecl->body->statements[0].get()))
					funType->returnType = Evaluate(retStmt->expr.get());
			}
			m_currentFunctionType = prevFun;
			m_env.PopScope();
		}
		else if (funType->returnType == nullptr && !realFunDecl->isExprBody)
		{
			funType->returnType = m_tUnit;
		}

		m_instantiatedFunctions[uniqueName] = funType;
		m_env.Define(uniqueName, funType, true);
		m_allFunctionNames.insert(uniqueName);

		m_pendingFunInstantiations.push_back(realFunDecl);
		m_instantiatedNodes.insert(realFunDecl);

		m_currentProgram->statements.push_back(std::move(clonedDecl));

		return funType;
	}
};

} // namespace igni::sem