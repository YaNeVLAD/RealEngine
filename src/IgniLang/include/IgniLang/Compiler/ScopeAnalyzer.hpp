#pragma once

#include <IgniLang/AST/AstNodes.hpp>

#include <unordered_map>
#include <unordered_set>

namespace igni
{

// ==========================================
// PASS 1: Scope & Capture Analyzer (Visitor)
// ==========================================
class ScopeAnalyzer final : public ast::BaseAstVisitor
{
public:
	ScopeAnalyzer(
		std::vector<const ast::FunDecl*>& flatFuncs,
		std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& funcUpvalues,
		std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& funcBoxedVars,
		const std::unordered_set<re::String>& globalNames)
		: m_flatFunctions(flatFuncs)
		, m_functionUpvalues(funcUpvalues)
		, m_functionBoxedVars(funcBoxedVars)
	{
		m_scopeStack.emplace_back(globalNames); // Global scope
	}

	void Analyze(const ast::Program* program)
	{
		program->Accept(*this);
	}

	void Visit(const ast::IndexExpr* node) override
	{
		if (node->array)
		{
			node->array->Accept(*this);
		}
		if (node->index)
		{
			node->index->Accept(*this);
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		if (!m_scopeStack.back().contains(node->name))
		{
			m_freeVars.insert(node->name);
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		if (node->left)
		{
			node->left->Accept(*this);
		}
		if (node->right)
		{
			node->right->Accept(*this);
		}
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
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (node->target)
		{
			node->target->Accept(*this);
		}
		if (node->value)
		{
			node->value->Accept(*this);
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		if (node->operand)
		{
			node->operand->Accept(*this);
		}
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
	}

	void Visit(const ast::Block* node) override
	{
		m_scopeStack.emplace_back(m_scopeStack.back());
		for (const auto& s : node->statements)
		{
			if (s)
			{
				s->Accept(*this);
			}
		}
		m_scopeStack.pop_back();
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
		if (node->startExpr)
		{
			node->startExpr->Accept(*this);
		}
		if (node->endExpr)
		{
			node->endExpr->Accept(*this);
		}

		m_scopeStack.emplace_back(m_scopeStack.back());
		m_scopeStack.back().insert(node->iteratorName);

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_scopeStack.pop_back();
	}

	// --- Объявления ---
	void Visit(const ast::VarDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		m_scopeStack.back().insert(node->name);
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		m_scopeStack.back().insert(node->name);
	}

	void Visit(const ast::FunDecl* node) override
	{
		m_scopeStack.back().insert(node->name);
		if (node->isExternal)
		{
			return;
		}

		m_functionBoxedVars[node];

		ScopeAnalyzer childAnalyzer(m_flatFunctions, m_functionUpvalues, m_functionBoxedVars, m_scopeStack.front());
		childAnalyzer.m_currentFunContext = node;

		std::unordered_set<re::String> funLocals;
		funLocals.insert(node->name);
		for (const auto& [name, type] : node->parameters)
		{
			funLocals.insert(name);
		}
		childAnalyzer.m_scopeStack.emplace_back(funLocals);

		if (node->body)
		{
			node->body->Accept(childAnalyzer);
		}

		std::vector<re::String> upvalues;
		for (const auto& uv : childAnalyzer.m_freeVars)
		{
			if (!m_scopeStack.front().contains(uv))
			{
				upvalues.emplace_back(uv);
				if (m_currentFunContext)
				{
					m_functionBoxedVars[m_currentFunContext].insert(uv);
				}
				if (!m_scopeStack.back().contains(uv))
				{
					m_freeVars.insert(uv);
				}
			}
		}

		m_functionUpvalues[node] = upvalues;
		m_flatFunctions.emplace_back(node);
	}

	void Visit(const ast::Program* node) override
	{
		for (const auto& stmt : node->statements)
		{
			if (stmt)
			{
				stmt->Accept(*this);
			}
		}
	}

private:
	std::vector<std::unordered_set<re::String>> m_scopeStack;
	std::unordered_set<re::String> m_freeVars;
	const ast::FunDecl* m_currentFunContext = nullptr;

	std::vector<const ast::FunDecl*>& m_flatFunctions;
	std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& m_functionUpvalues;
	std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& m_functionBoxedVars;
};

} // namespace igni