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
		const std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& funcUpvals,
		const std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& funcBoxedVars,
		const std::unordered_map<re::String, re::String>& importAliases)
		: m_out(out)
		, m_flatFunctions(flatFuncs)
		, m_functionUpvalues(funcUpvals)
		, m_functionBoxedVars(funcBoxedVars)
		, m_importAliases(importAliases)
	{
	}

	void Generate(const ast::Program* program)
	{
		m_out << "// ==========================================\n";
		m_out << "// Auto-generated RVM Assembly\n";
		m_out << "// Package: " << program->packageName << "\n";
		m_out << "// ==========================================\n\n";
		m_out << "// --- Entry Point ---\n";
		m_out << "CALL main 0\nRETURN\n\n";

		for (const ast::FunDecl* fun : m_flatFunctions)
		{
			GenerateFunction(fun);
		}
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		if (node->object)
		{
			node->object->Accept(*this);
		}

		m_out << "GET_PROPERTY \"" << node->member << "\"\n";
	}

	void Visit(const ast::IndexExpr* node) override
	{
		if (node->array)
		{
			node->array->Accept(*this); // Push self
		}
		if (node->index)
		{
			node->index->Accept(*this); // Push arg1
		}
		m_out << "CALL_METHOD \"get\" 1\n";
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
		const auto& name = node->name;
		if (m_importAliases.contains(name))
		{
			m_out << "GET_GLOBAL \"" << m_importAliases.at(name) << "\"\n";
			m_out << "GET_PROPERTY \"" << name << "\"\n";
			return;
		}

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
			const auto it = std::ranges::find_if(m_flatFunctions, [name](const ast::FunDecl* fun) {
				return fun->name == name;
			});
			if (it != m_flatFunctions.end())
			{
				m_out << "LOAD_FUN " << name << "\n";
			}
			else
			{
				m_out << "GET_GLOBAL \"" << name << "\"\n";
			}
		}
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			const auto& name = id->name;
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
		else if (const auto idx = dynamic_cast<const ast::IndexExpr*>(node->target.get()))
		{
			if (idx->array)
			{
				idx->array->Accept(*this); // self
			}
			if (idx->index)
			{
				idx->index->Accept(*this); // arg1
			}
			if (node->value)
			{
				node->value->Accept(*this); // arg2
			}
			m_out << "CALL_METHOD \"set\" 2\n";
			m_out << "CONST 0\n";
		}
	}

	void Visit(const ast::CallExpr* node) override
	{
		if (const auto memberAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			if (memberAccess->object)
			{
				memberAccess->object->Accept(*this);
			}

			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}
			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			m_out << "CALL_METHOD \"" << memberAccess->member << "\" " << actualArgs << "\n";
			return;
		}

		bool isDirectCall = false;
		re::String directFuncName;
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{
			directFuncName = id->name;
			const auto hashed = directFuncName.Hashed();

			if (m_importAliases.contains(directFuncName))
			{
				const auto& moduleName = m_importAliases.at(directFuncName);
				m_out << "GET_GLOBAL \"" << moduleName << "\"\n";

				for (const auto& arg : node->arguments)
				{
					if (arg)
						arg->Accept(*this);
				}

				if (node->isVarargCall)
				{
					m_out << "PACK_ARRAY " << node->varargCount << "\n";
				}
				const std::size_t actualArgs = node->isVarargCall
					? (node->arguments.size() - node->varargCount + 1)
					: node->arguments.size();

				m_out << "CALL_METHOD \"" << directFuncName << "\" " << actualArgs << "\n";
				return;
			}

			if (hashed == "print"_hs || hashed == "make_array"_hs || hashed == "len"_hs)
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

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}
			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			if (directFuncName == "print" || directFuncName == "make_array" || directFuncName == "len")
			{
				m_out << "NATIVE \"" << directFuncName << "\" " << actualArgs << "\n";
			}
			else
			{
				m_out << "CALL " << directFuncName << " " << actualArgs << "\n";
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

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}

			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			m_out << "CALL_INDIRECT " << actualArgs << "\n";
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		const auto& op = node->op;
		if (const auto hashed = op.Hashed(); hashed == "-"_hs)
		{
			m_out << "CONST 0\n";
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "SUB\n";
		}
		else if (hashed == "+"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
		}
		else if (hashed == "!"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "CONST 0\nEQUAL\n";
		}
		else if (hashed == "~"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "BIT_NOT\n";
		}
		else if (hashed == "++"_hs || hashed == "--"_hs)
		{
			const auto vmOp = (hashed == "++"_hs) ? "INC" : "DEC";
			const auto revOp = (hashed == "++"_hs) ? "DEC" : "INC";

			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->operand.get()))
			{
				const auto& name = id->name;
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
		case ">"_hs:  m_out << "GREATER\n"; break;
		case "<="_hs: m_out << "LESS_EQUAL\n"; break;
		case ">="_hs: m_out << "GREATER_EQUAL\n"; break;
		case "=="_hs: m_out << "EQUAL\n"; break;
		case "!="_hs: m_out << "NOT_EQUAL\n"; break;
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
				if (id->name.Hashed() == "print"_hs)
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
		const re::String elseLabel = "L_else_" + std::to_string(currentLabelId);
		const re::String endLabel = "L_end_" + std::to_string(currentLabelId);

		m_out << "// --- if ---\n";
		if (node->condition)
		{
			node->condition->Accept(*this);
		}

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
		const re::String startLabel = "L_while_start_" + std::to_string(labelId);
		const re::String endLabel = "L_while_end_" + std::to_string(labelId);

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
		const re::String startLabel = "L_for_start_" + std::to_string(labelId);
		const re::String endLabel = "L_for_end_" + std::to_string(labelId);

		const auto& iterName = node->iteratorName;
		re::String limitName = "_for_limit_" + std::to_string(labelId);

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

		if (m_functionBoxedVars.at(m_currentFunction).contains(node->name))
		{
			m_out << "BOX\n";
		}

		m_out << "SET " << node->name << "\n";
		m_currentLocals.emplace_back(node->name);
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

		if (m_functionBoxedVars.at(m_currentFunction).contains(node->name))
		{
			m_out << "BOX\n";
		}

		m_out << "SET " << node->name << "\n";
		m_currentLocals.emplace_back(node->name);
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

		m_out << "MAKE_CLOSURE " << node->name << " " << upvalues.size() << "\n";
		m_out << "SET " << node->name << "\n";
		m_currentLocals.emplace_back(node->name);
	}

private:
	std::ostream& m_out;
	std::size_t m_labelCount = 0;

	const ast::FunDecl* m_currentFunction = nullptr;
	std::vector<re::String> m_currentLocals;
	std::vector<re::String> m_currentUpvalues;

	const std::vector<const ast::FunDecl*>& m_flatFunctions;
	const std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& m_functionUpvalues;
	const std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& m_functionBoxedVars;

	const std::unordered_map<re::String, re::String>& m_importAliases;

	void GenerateFunction(const ast::FunDecl* fun)
	{
		m_currentFunction = fun;
		m_out << "FUN " << fun->name << "\n";
		m_currentLocals.clear();
		m_currentUpvalues = m_functionUpvalues.at(fun);

		const auto& boxedVars = m_functionBoxedVars.at(fun);

		for (auto it = fun->parameters.rbegin(); it != fun->parameters.rend(); ++it)
		{
			auto paramName = it->name;
			if (boxedVars.contains(paramName))
			{
				m_out << "BOX\n";
			}
			m_out << "SET " << paramName << "\n";
			m_currentLocals.emplace_back(paramName);
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