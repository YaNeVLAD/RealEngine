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

	void Analyze(const ast::Program* program)
	{
		program->Accept(*this);
	}

	[[nodiscard]] std::unordered_set<re::String> GetGlobalNames() const
	{
		return m_env.GetGlobalNames();
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
			throw std::runtime_error("Semantic Error: Undefined variable '" + node->name.ToString() + "'");
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
			m_currentExprType = ResolveMethod(classType, node->member);
		}
		else
		{
			throw std::runtime_error("Semantic Error: Cannot access member '" + node->member.ToString() + "' on primitive type '" + leftType->name + "'");
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		const auto leftType = Evaluate(node->left.get());
		const auto rightType = Evaluate(node->right.get());

		ExpectAssignable(rightType, leftType, "binary expression");

		if (const auto hashed = node->op.Hashed();
			hashed == "<"_hs || hashed == ">"_hs
			|| hashed == "=="_hs || hashed == "!="_hs
			|| hashed == "<="_hs || hashed == ">="_hs)
		{
			m_currentExprType = m_tBool;
		}
		else
		{
			m_currentExprType = leftType;
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

		const auto funType = std::dynamic_pointer_cast<FunctionType>(calleeType);
		if (!funType)
		{
			throw std::runtime_error("Semantic Error: Attempt to call a non-callable type");
		}

		ValidateCallArguments(node, funType);
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

	void Visit(const ast::AssignExpr* node) override
	{
		const auto targetType = Evaluate(node->target.get());
		const auto valueType = Evaluate(node->value.get());

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

		if (m_currentReturnType)
		{
			ExpectAssignable(retType, m_currentReturnType, "return statement");
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		const Symbol* sym = m_env.Resolve(node->name);
		const auto funType = std::dynamic_pointer_cast<FunctionType>(sym->type);

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
		m_currentReturnType = funType->returnType;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_currentReturnType = previousReturnType;
		m_env.PopScope();
	}

	void Visit(const ast::ImportDecl* node) override
	{
		ProcessImport(node);
	}

	void Visit(const ast::Program* node) override
	{
		for (const auto& stmt : node->statements)
		{ // Functions forward declarations
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				auto funType = std::make_shared<FunctionType>(fun->name);
				funType->returnType = ResolveAstType(fun->returnType.get());
				funType->isVararg = fun->isVararg;

				for (const auto& [name, type] : fun->parameters)
				{
					funType->paramTypes.emplace_back(ResolveAstType(type.get()));
				}
				m_env.Define(fun->name, funType, true);
				if (fun->isExternal)
				{
					m_externalFunctions.insert(fun->name);
				}
			}
		}

		for (const auto& import : node->imports)
		{
			if (import)
			{
				import->Accept(*this);
			}
		}

		for (const auto& stmt : node->statements)
		{
			if (stmt)
			{
				stmt->Accept(*this);
			}
		}
	}

private:
	Environment m_env;
	std::shared_ptr<SemanticType> m_currentExprType = nullptr;
	std::shared_ptr<SemanticType> m_currentReturnType = nullptr;

	std::unordered_map<re::String, re::String> m_importAliases;
	std::unordered_set<re::String> m_externalFunctions;

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
			throw std::runtime_error("Semantic Error: Cannot infer type for variable '" + name.ToString() + "'");
		}

		m_env.Define(name, varType, isReadOnly);
	}

	static std::shared_ptr<SemanticType> ResolveExport(const std::shared_ptr<ModuleType>& modType, const re::String& member)
	{
		if (const auto it = modType->exports.find(member.ToString()); it != modType->exports.end())
		{
			return it->second;
		}
		throw std::runtime_error("Semantic Error: Module '" + modType->name + "' has no export named '" + member.ToString() + "'");
	}

	static std::shared_ptr<SemanticType> ResolveMethod(const std::shared_ptr<ClassType>& classType, const re::String& member)
	{
		if (const auto it = classType->methods.find(member.ToString()); it != classType->methods.end())
		{
			return it->second;
		}
		throw std::runtime_error("Semantic Error: Class '" + classType->name + "' has no method named '" + member.ToString() + "'");
	}

	void ValidateCallArguments(const ast::CallExpr* node, const std::shared_ptr<FunctionType>& funType)
	{
		if (funType->isVararg)
		{
			const std::size_t fixedParams = funType->paramTypes.size() - 1;
			if (node->arguments.size() < fixedParams)
			{
				throw std::runtime_error("Semantic Error: Not enough arguments for vararg function");
			}

			const auto mutableNode = const_cast<ast::CallExpr*>(node);
			mutableNode->isVarargCall = true;
			mutableNode->varargCount = node->arguments.size() - fixedParams;
		}
		else if (node->arguments.size() != funType->paramTypes.size())
		{
			throw std::runtime_error("Semantic Error: Argument count mismatch");
		}

		for (std::size_t i = 0; i < node->arguments.size(); ++i)
		{
			const auto argType = Evaluate(node->arguments[i].get());

			std::shared_ptr<SemanticType> expectedType = (funType->isVararg && i >= funType->paramTypes.size() - 1)
				? funType->paramTypes.back()
				: funType->paramTypes[i];

			ExpectAssignable(argType, expectedType, "argument " + std::to_string(i + 1));
		}
	}

	void ProcessImport(const ast::ImportDecl* node)
	{
		const re::String fullPath = node->path;
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

		if (node->isStar)
		{
			for (const auto& [exportName, exportType] : modType->exports)
			{
				m_env.Define(exportName, exportType, true);
				m_importAliases[exportName] = modName;
			}
		}
		else
		{
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
		{
			return m_tUnit;
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

			throw std::runtime_error("Semantic Error: Unknown type '" + simpleType->name.ToString() + "'");
		}

		return m_tUnit;
	}
};

} // namespace igni::sem