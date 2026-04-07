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
			RegisterProgramDeclarations(prog.get());
		}

		for (const auto& prog : programs)
		{
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

			for (const auto& stmt : prog->statements)
			{
				if (stmt)
				{
					stmt->Accept(*this);
				}
			}

			m_env.PopScope();
		}
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
		if (const auto arrType = Evaluate(node->array.get()); arrType->name != "Array")
		{
			throw std::runtime_error("Semantic Error: Cannot index non-array type '" + arrType->name + "'");
		}

		const auto indexType = Evaluate(node->index.get());
		ExpectAssignable(indexType, m_tInt, "array index");

		// TODO: Replace with generic element type when Generics are introduced
		m_currentExprType = m_tAny;
	}

	void Visit(const ast::CallExpr* node) override
	{
		const auto calleeType = Evaluate(node->callee.get());

		if (const auto classType = std::dynamic_pointer_cast<ClassType>(calleeType))
		{
			const auto mutableNode = const_cast<ast::CallExpr*>(node);
			mutableNode->isConstructorCall = true;

			if (const auto it = classType->methods.find(classType->name); it != classType->methods.end())
			{
				const auto ctorType = std::dynamic_pointer_cast<FunctionType>(it->second);

				ValidateCallArguments(node, ctorType, true);

				mutableNode->staticMethodTarget = ctorType->name;
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
				mutableNode->staticMethodTarget = classType->name + "_" + memAccess->member;
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
		const Symbol* sym = m_env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

		m_env.PushScope();

		// Parameters including 'this'
		for (std::size_t i = 0; i < node->parameters.size(); ++i)
		{
			std::shared_ptr<SemanticType> paramType = funType->paramTypes[i];
			if (node->isVararg && i == node->parameters.size() - 1)
			{
				paramType = m_tArray;
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
				paramType = m_tArray;
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
		const Symbol* sym = m_env.Resolve(node->name);
		const auto classType = std::dynamic_pointer_cast<ClassType>(sym->type);

		const auto previousClass = m_currentClassType;
		m_currentClassType = classType;

		for (const auto& member : node->members)
		{ // Register member values
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{ // Member var
				auto fieldType = ResolveAstType(varDecl->type.get());
				if (varDecl->initializer)
				{ // Resolve a field type from initializer
					auto initType = Evaluate(varDecl->initializer.get());
					if (fieldType == nullptr || fieldType == m_tUnit)
					{
						fieldType = initType;
					}
					else
					{
						ExpectAssignable(initType, fieldType, "field initialization");
					}
				}
				classType->fields[varDecl->name] = { fieldType, false, varDecl->visibility };
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{ // Member val
				auto fieldType = ResolveAstType(valDecl->type.get());
				if (valDecl->initializer)
				{ // Resolve a field type from initializer
					auto initType = Evaluate(valDecl->initializer.get());
					if (fieldType == nullptr || fieldType == m_tUnit)
					{
						fieldType = initType;
					}
					else
					{
						ExpectAssignable(initType, fieldType, "field initialization");
					}
				}
				classType->fields[valDecl->name] = { fieldType, true, valDecl->visibility };
			}
		}

		for (const auto& member : node->members)
		{ // Register member functions
			if (const auto funDecl = dynamic_cast<const ast::FunDecl*>(member.get()))
			{
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

	std::unordered_map<re::String, re::String> m_importAliases;
	std::unordered_set<re::String> m_externalFunctions;
	std::unordered_set<re::String> m_allFunctionNames;

	// --- Type Definitions ---
	std::shared_ptr<PrimitiveType> m_tInt = std::make_shared<PrimitiveType>("Int");
	std::shared_ptr<PrimitiveType> m_tDouble = std::make_shared<PrimitiveType>("Double");
	std::shared_ptr<PrimitiveType> m_tBool = std::make_shared<PrimitiveType>("Bool");
	std::shared_ptr<PrimitiveType> m_tString = std::make_shared<PrimitiveType>("String");
	std::shared_ptr<PrimitiveType> m_tUnit = std::make_shared<PrimitiveType>("Unit");
	std::shared_ptr<PrimitiveType> m_tAny = std::make_shared<PrimitiveType>("Any");
	std::shared_ptr<ClassType> m_tArray = std::make_shared<ClassType>("Array");

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
		m_env.Define("Array", m_tArray, true);
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
				auto classType = std::make_shared<ClassType>(classDecl->name);

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

				// --- ИСПРАВЛЕНИЕ: Кладем ТОЛЬКО в m_env ИЛИ ТОЛЬКО в exports ---
				if (isGlobal)
				{
					m_env.Define(classDecl->name, classType, true);
				}
				else
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

						// --- ИСПРАВЛЕНИЕ: Исправлен баг с classType и логика Scopes ---
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
				return sym->type;
			}

			if (m_currentPackage)
			{
				if (const auto it = m_currentPackage->exports.find(simpleType->name); it != m_currentPackage->exports.end())
				{
					return it->second;
				}
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
};

} // namespace igni::sem