#pragma once

#include <Core/HashedString.hpp>
#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

#include <unordered_map>
#include <unordered_set>

namespace igni
{

// ==========================================
// PASS 2: Code Generator (Visitor)
// ==========================================
class CodeGenerator final : public ast::BaseAstVisitor
{
public:
	CodeGenerator(
		std::ostream& out,
		const std::vector<const ast::FunDecl*>& flatFuncs,
		const std::unordered_map<const ast::FunDecl*, std::vector<std::string>>& funcUpvals,
		const std::unordered_map<const ast::FunDecl*, std::unordered_set<std::string>>& funcBoxedVars)
		: m_out(out)
		, m_flatFunctions(flatFuncs)
		, m_functionUpvalues(funcUpvals)
		, m_functionBoxedVars(funcBoxedVars)
	{
	}

	void Generate(const ast::Program* program)
	{
		m_out << "// ==========================================\n";
		m_out << "// Auto-generated RVM Assembly\n";
		m_out << "// Package: " << program->packageName.ToString() << "\n";
		m_out << "// ==========================================\n\n";
		m_out << "// --- Entry Point ---\n";
		m_out << "CALL main 0\nRETURN\n\n";

		for (const ast::FunDecl* fun : m_flatFunctions)
		{
			GenerateFunction(fun);
		}
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		if (node->token.type == TokenType::KwTrue)
		{
			m_out << "CONST 1\n";
		}
		else if (node->token.type == TokenType::KwFalse)
		{
			m_out << "CONST 0\n";
		}
		else
		{
			m_out << "CONST " << node->token.lexeme << "\n";
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		const auto name = node->name.ToString();
		if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
		{
			m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
		}
		else if (std::ranges::find(m_currentLocals, name) != m_currentLocals.end())
		{
			m_out << "GET " << name << "\n";
			if (m_functionBoxedVars.at(m_currentFunction).contains(name))
			{
				m_out << "UNBOX\n";
			}
		}
		else
		{
			m_out << "LOAD_FUN " << name << "\n";
		}
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			const std::string name = id->name.ToString();
			if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
			{
				if (node->value)
				{
					node->value->Accept(*this);
				}
				m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else
			{
				if (m_functionBoxedVars.at(m_currentFunction).contains(name))
				{
					m_out << "GET " << name << "\n";
					if (node->value)
					{
						node->value->Accept(*this);
					}
					m_out << "STORE_BOX\n";
				}
				else
				{
					if (node->value)
					{
						node->value->Accept(*this);
					}
					m_out << "SET " << name << "\n";
				}
			}
			m_out << "CONST 0\n";
		}
	}

	void Visit(const ast::CallExpr* node) override
	{
		bool isDirectCall = false;
		std::string directFuncName = "";

		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{
			directFuncName = id->name.ToString();
			if (directFuncName == "print" || directFuncName == "make_array" || directFuncName == "len")
			{
				isDirectCall = true;
			}
			else if (std::ranges::find(m_currentLocals, directFuncName) == m_currentLocals.end()
				&& std::ranges::find(m_currentUpvalues, directFuncName) == m_currentUpvalues.end())
			{
				isDirectCall = true;
			}
		}

		if (isDirectCall)
		{
			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (directFuncName == "print" || directFuncName == "len")
			{
				m_out << "NATIVE \"" << directFuncName << "\" " << node->arguments.size() << "\n";
			}
			else
			{
				m_out << "CALL " << directFuncName << " " << node->arguments.size() << "\n";
			}
		}
		else
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
			m_out << "CALL_INDIRECT " << node->arguments.size() << "\n";
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		if (const std::string op = node->op.ToString(); op == "-")
		{
			m_out << "CONST 0\n";
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "SUB\n";
		}
		else if (op == "+")
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
		}
		else if (op == "!")
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "CONST 0\nEQUAL\n";
		}
		else if (op == "~")
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "BIT_NOT\n";
		}
		else if (op == "++" || op == "--")
		{
			const auto vmOp = (op == "++") ? "INC" : "DEC";
			const auto revOp = (op == "++") ? "DEC" : "INC";

			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->operand.get()))
			{
				const auto name = id->name.ToString();
				const auto upIt = std::ranges::find(m_currentUpvalues, name);

				if (upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
					m_out << vmOp << "\n";
					m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else if (m_functionBoxedVars.at(m_currentFunction).contains(name))
				{
					m_out << "GET " << name << "\nGET " << name << "\nUNBOX\n"
						  << vmOp << "\nSTORE_BOX\n";
				}
				else
				{
					m_out << "GET " << name << "\n"
						  << vmOp << "\nSET " << name << "\n";
				}

				if (upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					m_out << "GET " << name << "\n";
					if (m_functionBoxedVars.at(m_currentFunction).contains(name))
					{
						m_out << "UNBOX\n";
					}
				}

				if (node->isPostfix)
				{
					m_out << revOp << "\n";
				}
			}
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		if (node->left)
		{
			node->left->Accept(*this);
		}
		if (node->right)
		{
			node->right->Accept(*this);
		}

		// clang-format off
		switch (node->op.Hash())
		{
		case "+"_hs:  m_out << "ADD\n"; break;
		case "-"_hs:  m_out << "SUB\n"; break;
		case "*"_hs:  m_out << "MUL\n"; break;
		case "/"_hs:  m_out << "DIV\n"; break;
		case "<"_hs:  m_out << "LESS\n"; break;
		case "=="_hs: m_out << "EQUAL\n"; break;
		case "&&"_hs: m_out << "AND\n"; break;
		case "||"_hs: m_out << "OR\n"; break;
		default: throw std::runtime_error("TextCompiler: Unsupported binary operator '" + node->op.ToString() + "'");
		}
		// clang-format on
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}

		bool shouldPop = true;
		if (const auto call = dynamic_cast<const ast::CallExpr*>(node->expr.get()))
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(call->callee.get()))
			{
				if (id->name.ToString() == "print")
				{
					shouldPop = false;
				}
			}
		}
		if (shouldPop)
		{
			m_out << "POP\n";
		}
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}
		m_out << "RETURN\n";
	}

	void Visit(const ast::Block* node) override
	{
		for (const auto& s : node->statements)
		{
			if (s)
			{
				s->Accept(*this);
			}
		}
	}

	void Visit(const ast::IfStmt* node) override
	{
		const auto currentLabelId = m_labelCount++;
		const auto elseLabel = "L_else_" + std::to_string(currentLabelId);
		const auto endLabel = "L_end_" + std::to_string(currentLabelId);

		m_out << "// --- if ---\n";
		if (node->condition)
			node->condition->Accept(*this);

		m_out << "JMP_IF_FALSE " << (node->elseBranch ? elseLabel : endLabel) << "\n";

		if (node->thenBranch)
		{
			node->thenBranch->Accept(*this);
		}

		if (node->elseBranch)
		{
			m_out << "JMP " << endLabel << "\nLABEL " << elseLabel << "\n";
			node->elseBranch->Accept(*this);
		}
		m_out << "LABEL " << endLabel << "\n";
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const auto labelId = m_labelCount++;
		const auto startLabel = "L_while_start_" + std::to_string(labelId);
		const auto endLabel = "L_while_end_" + std::to_string(labelId);

		m_out << "LABEL " << startLabel << "\n";
		if (node->condition)
		{
			node->condition->Accept(*this);
		}
		m_out << "JMP_IF_FALSE " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_out << "JMP " << startLabel << "\nLABEL " << endLabel << "\n";
	}

	void Visit(const ast::ForStmt* node) override
	{
		const auto labelId = m_labelCount++;
		const auto startLabel = "L_for_start_" + std::to_string(labelId);
		const auto endLabel = "L_for_end_" + std::to_string(labelId);

		auto iterName = node->iteratorName.ToString();
		auto limitName = "_for_limit_" + std::to_string(labelId);

		m_out << "// --- for (" << iterName << ") ---\n";
		if (node->startExpr)
		{
			node->startExpr->Accept(*this);
		}
		m_out << "SET " << iterName << "\n";
		m_currentLocals.emplace_back(iterName);

		if (node->endExpr)
		{
			node->endExpr->Accept(*this);
		}
		m_out << "SET " << limitName << "\n";
		m_currentLocals.emplace_back(limitName);

		m_out << "LABEL " << startLabel << "\n";
		m_out << "CONST 1\nGET " << limitName << "\nGET " << iterName << "\nLESS\nSUB\n";
		m_out << "JMP_IF_FALSE " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_out << "GET " << iterName << "\nCONST 1\nADD\nSET " << iterName << "\n";
		m_out << "JMP " << startLabel << "\nLABEL " << endLabel << "\n";
	}

	void Visit(const ast::VarDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}

		if (m_functionBoxedVars.at(m_currentFunction).contains(node->name.ToString()))
		{
			m_out << "BOX\n";
		}

		m_out << "SET " << node->name.ToString() << "\n";
		m_currentLocals.push_back(node->name.ToString());
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}

		if (m_functionBoxedVars.at(m_currentFunction).contains(node->name.ToString()))
		{
			m_out << "BOX\n";
		}

		m_out << "SET " << node->name.ToString() << "\n";
		m_currentLocals.push_back(node->name.ToString());
	}

	void Visit(const ast::FunDecl* node) override
	{
		const auto& upvalues = m_functionUpvalues.at(node);
		for (const auto& uv : upvalues)
		{
			if (auto upIt = std::ranges::find(m_currentUpvalues, uv); upIt != m_currentUpvalues.end())
			{
				m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else
			{
				m_out << "GET " << uv << "\n";
			}
		}

		m_out << "MAKE_CLOSURE " << node->name.ToString() << " " << upvalues.size() << "\n";
		m_out << "SET " << node->name.ToString() << "\n";
		m_currentLocals.push_back(node->name.ToString());
	}

private:
	std::ostream& m_out;
	std::size_t m_labelCount = 0;

	const ast::FunDecl* m_currentFunction = nullptr;
	std::vector<std::string> m_currentLocals;
	std::vector<std::string> m_currentUpvalues;

	const std::vector<const ast::FunDecl*>& m_flatFunctions;
	const std::unordered_map<const ast::FunDecl*, std::vector<std::string>>& m_functionUpvalues;
	const std::unordered_map<const ast::FunDecl*, std::unordered_set<std::string>>& m_functionBoxedVars;

	void GenerateFunction(const ast::FunDecl* fun)
	{
		m_currentFunction = fun;
		m_out << "FUN " << fun->name.ToString() << "\n";
		m_currentLocals.clear();
		m_currentUpvalues = m_functionUpvalues.at(fun);

		const auto& boxedVars = m_functionBoxedVars.at(fun);

		for (auto it = fun->parameters.rbegin(); it != fun->parameters.rend(); ++it)
		{
			std::string paramName = it->name.ToString();
			if (boxedVars.contains(paramName))
			{
				m_out << "BOX\n";
			}
			m_out << "SET " << paramName << "\n";
			m_currentLocals.push_back(paramName);
		}

		if (fun->body)
		{
			for (const auto& s : fun->body->statements)
			{ // Avoiding recursive code generation by manually calling Accept for function statements
				if (s)
				{
					s->Accept(*this);
				}
			}
		}

		m_out << "CONST 0\nRETURN\n\n";
	}
};

} // namespace igni