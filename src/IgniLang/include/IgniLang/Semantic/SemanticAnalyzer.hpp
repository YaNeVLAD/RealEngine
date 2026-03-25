#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>
#include <RVM/VirtualMachine.hpp>

namespace igni::sem
{

class SemanticAnalyzer final : public ast::BaseAstVisitor
{
public:
	SemanticAnalyzer()
	{
		using namespace re::rvm;
		// Registering native functions
		const auto printType = std::make_shared<FunctionType>();
		printType->returnType = m_tUnit;
		m_env.Define("print", printType, true);

		// Manually adding module (import std.math)
		const auto mathModule = std::make_shared<ModuleType>("math");
		const auto sqrtType = std::make_shared<FunctionType>();
		sqrtType->paramTypes.emplace_back(m_tDouble);
		sqrtType->returnType = m_tDouble;
		mathModule->exports["sqrt"] = sqrtType;

		m_env.Define("math", mathModule, true);

		const auto makeArrayType = std::make_shared<FunctionType>();
		// TODO: Return ClassType("Array")
		m_env.Define("make_array", makeArrayType, true);

		const auto lenType = std::make_shared<FunctionType>();
		lenType->returnType = m_tInt;
		m_env.Define("len", lenType, true);
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

	// ==========================================
	// EXPRESSIONS
	// ==========================================
	void Visit(const ast::LiteralExpr* node) override
	{
		// clang-format off
        if (node->token.type == TokenType::IntConst) m_currentExprType = m_tInt;
        else if (node->token.type == TokenType::FloatConst) m_currentExprType = m_tDouble;
        else if (node->token.type == TokenType::StringConst) m_currentExprType = m_tString;
        else if (node->token.type == TokenType::KwTrue || node->token.type == TokenType::KwFalse) m_currentExprType = m_tBool;
        else m_currentExprType = m_tUnit;
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
		if (node->object)
		{
			node->object->Accept(*this);
		}

		const auto leftType = m_currentExprType;
		if (const auto modType = std::dynamic_pointer_cast<ModuleType>(leftType))
		{
			const auto it = modType->exports.find(node->member.ToString());
			if (it == modType->exports.end())
			{
				throw std::runtime_error("Semantic Error: Module '" + modType->name + "' has no export named '" + node->member.ToString() + "'");
			}
			m_currentExprType = it->second;
		}
		else if (const auto classType = std::dynamic_pointer_cast<ClassType>(leftType))
		{
			const auto it = classType->methods.find(node->member.ToString());
			if (it == classType->methods.end())
			{
				throw std::runtime_error("Semantic Error: Class '" + classType->name + "' has no method named '" + node->member.ToString() + "'");
			}
			m_currentExprType = it->second;
		}
		else
		{
			throw std::runtime_error("Semantic Error: Cannot access member '" + node->member.ToString() + "' on primitive type '" + leftType->name + "'");
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		node->left->Accept(*this);
		const auto leftType = m_currentExprType;

		node->right->Accept(*this);
		const auto rightType = m_currentExprType;

		if (!leftType->IsAssignableTo(rightType.get()))
		{
			throw std::runtime_error("Semantic Error: Type mismatch in binary expression (" + leftType->name + " and " + rightType->name + ")");
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
			m_currentExprType = leftType;
		}
	}

	// ==========================================
	// INSTRUCTIONS
	// ==========================================
	void Visit(const ast::VarDecl* node) override
	{
		std::shared_ptr<SemanticType> varType = nullptr;

		if (node->initializer)
		{
			node->initializer->Accept(*this);
			varType = m_currentExprType;
		}

		// TODO: Check explicit user type for matching (node->type)

		if (!varType)
		{ // Fallback
			varType = m_tUnit;
		}

		m_env.Define(node->name, varType, false);
	}

	void Visit(const ast::ValDecl* node) override
	{
		std::shared_ptr<SemanticType> valType = nullptr;
		if (node->initializer)
		{
			node->initializer->Accept(*this);
			valType = m_currentExprType;
		}

		// TODO: Check explicit user type for matching (node->type)

		if (!valType)
		{ // Fallback
			valType = m_tUnit;
		}

		m_env.Define(node->name, valType, true);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (node->target)
		{
			node->target->Accept(*this);
		}
		const auto targetType = m_currentExprType;

		if (node->value)
		{
			node->value->Accept(*this);
		}
		const auto valueType = m_currentExprType;

		if (!valueType->IsAssignableTo(targetType.get()))
		{
			throw std::runtime_error("Semantic Error: Cannot assign " + valueType->name + " to " + targetType->name);
		}
		m_currentExprType = valueType;
	}

	void Visit(const ast::IndexExpr* node) override
	{
		if (node->array)
		{
			node->array->Accept(*this);
		}
		// TODO: Add check that array is ClassType("Array")
		if (node->index)
		{
			node->index->Accept(*this);
		}
		// TODO: Replace Int with Generics
		m_currentExprType = m_tInt;
	}

	void Visit(const ast::IfStmt* node) override
	{
		if (node->condition)
		{
			node->condition->Accept(*this);
		}
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
		if (node->condition)
		{
			node->condition->Accept(*this);
		}
		if (node->body)
		{
			node->body->Accept(*this);
		}
	}

	void Visit(const ast::ForStmt* node) override
	{
		m_env.PushScope();

		if (node->startExpr)
		{
			node->startExpr->Accept(*this);
		}
		if (node->endExpr)
		{
			node->endExpr->Accept(*this);
		}

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
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		if (node->operand)
		{
			node->operand->Accept(*this);
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		m_env.PushScope(); // Scope аргументов функции

		for (const auto& [name, type] : node->parameters)
		{
			m_env.Define(name.ToString(), m_tUnit, true); // Пока заглушка
		}

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_env.PopScope();
	}

	void Visit(const ast::CallExpr* node) override
	{
		if (node->callee)
		{
			node->callee->Accept(*this);
		}
		for (const auto& arg : node->arguments)
		{
			if (arg)
			{
				arg->Accept(*this);
			}
		}
		m_currentExprType = m_tUnit;
	}

	void Visit(const ast::ImportDecl* node) override
	{
		std::string fullPath = node->path.ToString();

		const std::size_t dotPos = fullPath.find('.');
		if (dotPos == std::string::npos)
		{ // import module;
			return;
		}

		const std::string modName = fullPath.substr(0, dotPos);
		const std::string memberName = fullPath.substr(dotPos + 1);

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
		{ // import module.*
			for (const auto& [exportName, exportType] : modType->exports)
			{
				m_env.Define(exportName, exportType, true);

				m_importAliases[exportName] = modName;
			}
		}
		else
		{ // import module.ident
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

	void Visit(const ast::Program* node) override
	{
		for (const auto& stmt : node->statements)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				auto funType = std::make_shared<FunctionType>();
				funType->returnType = m_tUnit; // Пока заглушка
				m_env.Define(fun->name.ToString(), funType, true);
			}
		}

		for (const auto& imp : node->imports)
		{
			if (imp)
			{
				imp->Accept(*this);
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

	std::unordered_map<re::String, re::String> m_importAliases;

	std::shared_ptr<PrimitiveType> m_tInt = std::make_shared<PrimitiveType>("Int");
	std::shared_ptr<PrimitiveType> m_tDouble = std::make_shared<PrimitiveType>("Double");
	std::shared_ptr<PrimitiveType> m_tBool = std::make_shared<PrimitiveType>("Bool");
	std::shared_ptr<PrimitiveType> m_tString = std::make_shared<PrimitiveType>("String");
	std::shared_ptr<PrimitiveType> m_tUnit = std::make_shared<PrimitiveType>("Unit");
};

} // namespace igni::sem