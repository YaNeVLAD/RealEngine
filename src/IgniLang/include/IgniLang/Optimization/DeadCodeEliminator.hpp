#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace igni::opt
{

class DeadCodeEliminator final : public ast::BaseAstVisitor
{
public:
	// Принимает только корень программы и мутирует его in-place
	void Eliminate(ast::Program* program)
	{
		m_reachableNames.clear();
		m_worklist.clear();
		m_allBodies.clear();

		// 1. Построение карты тел функций напрямую из AST
		for (const auto& stmt : program->statements)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				if (fun->typeParams.empty())
				{
					m_allBodies[fun->name] = fun->body.get();
				}
			}
			else if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				if (classDecl->typeParams.empty())
				{
					for (const auto& member : classDecl->members)
					{
						if (const auto mFun = dynamic_cast<const ast::FunDecl*>(member.get()))
						{
							m_allBodies[mFun->name] = mFun->body.get();
						}
						else if (const auto mCtor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
						{
							m_allBodies[mCtor->name] = mCtor->body.get();
						}
						else if (const auto mDtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
						{
							m_allBodies[mDtor->name] = mDtor->body.get();
						}
					}
				}
			}
		}

		// 2. Mark: Инициируем поиск с корней (main)
		MarkReached("main");

		// Анализируем глобальные инициализации (val, var, expr)
		for (const auto& stmt : program->statements)
		{
			if (!dynamic_cast<const ast::FunDecl*>(stmt.get()) && !dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				stmt->Accept(*this);
			}
		}

		// Волновой обход графа вызовов
		while (!m_worklist.empty())
		{
			const re::String current = m_worklist.back();
			m_worklist.pop_back();

			if (m_allBodies.contains(current) && m_allBodies.at(current))
			{
				m_allBodies.at(current)->Accept(*this);
			}
		}

		// 3. Sweep: ФИЗИЧЕСКОЕ УДАЛЕНИЕ МЕРТВОГО КОДА ИЗ AST

		// Очищаем глобальные функции
		std::erase_if(program->statements, [&](const std::unique_ptr<ast::Statement>& stmt) {
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				// Удаляем инстанцированную функцию, если ее нет в списке достижимых
				return fun->typeParams.empty() && !m_reachableNames.contains(fun->name);
			}

			return false; // Классы, переменные и выражения оставляем
		});

		// Очищаем методы классов прямо в AST
		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<ast::ClassDecl*>(stmt.get()))
			{
				if (!classDecl->typeParams.empty())
				{
					continue; // Сырые шаблоны не трогаем
				}

				std::erase_if(classDecl->members, [&](const std::unique_ptr<ast::Decl>& member) {
					if (const auto mFun = dynamic_cast<const ast::FunDecl*>(member.get()))
					{
						return !m_reachableNames.contains(mFun->name);
					}
					if (const auto mCtor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						return !m_reachableNames.contains(mCtor->name);
					}
					if (const auto mDtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
					{
						return !m_reachableNames.contains(mDtor->name);
					}

					return false; // VarDecl и ValDecl остаются!
				});
			}
		}
	}

private:
	std::unordered_set<re::String> m_reachableNames;
	std::vector<re::String> m_worklist;
	std::unordered_map<re::String, const ast::Block*> m_allBodies;

	void MarkReached(const re::String& name)
	{
		if (!m_reachableNames.contains(name))
		{
			m_reachableNames.insert(name);
			m_worklist.push_back(name);
		}
	}

#define VISIT_NEXT(node) \
	if (node)            \
	node->Accept(*this)

	void Visit(const ast::CallExpr* node) override
	{
		if (!node->staticMethodTarget.Empty())
		{
			MarkReached(node->staticMethodTarget);

			// Если вызываем конструктор, помечаем деструктор как живой
			if (node->isConstructorCall)
			{
				if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
				{
					MarkReached(id->name + "_destructor");
				}
			}
		}
		else if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{
			MarkReached(id->name);
		}
		else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			MarkReached(memAccess->member);
		}

		VISIT_NEXT(node->callee);
		for (const auto& arg : node->arguments)
		{
			VISIT_NEXT(arg);
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		MarkReached(node->name);
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		MarkReached(node->member);
		VISIT_NEXT(node->object);
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		VISIT_NEXT(node->left);
		VISIT_NEXT(node->right);
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		VISIT_NEXT(node->operand);
	}

	void Visit(const ast::IndexExpr* node) override
	{
		VISIT_NEXT(node->array);
		VISIT_NEXT(node->index);
	}

	void Visit(const ast::AssignExpr* node) override
	{
		VISIT_NEXT(node->target);
		VISIT_NEXT(node->value);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		VISIT_NEXT(node->expr);
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		VISIT_NEXT(node->expr);
	}

	void Visit(const ast::Block* node) override
	{
		for (const auto& s : node->statements)
		{
			VISIT_NEXT(s);
		}
	}

	void Visit(const ast::IfStmt* node) override
	{
		VISIT_NEXT(node->condition);
		VISIT_NEXT(node->thenBranch);
		VISIT_NEXT(node->elseBranch);
	}

	void Visit(const ast::WhileStmt* node) override
	{
		VISIT_NEXT(node->condition);
		VISIT_NEXT(node->body);
	}

	void Visit(const ast::ForStmt* node) override
	{
		VISIT_NEXT(node->startExpr);
		VISIT_NEXT(node->endExpr);
		VISIT_NEXT(node->body);
	}

	void Visit(const ast::VarDecl* node) override
	{
		VISIT_NEXT(node->initializer);
	}

	void Visit(const ast::ValDecl* node) override
	{
		VISIT_NEXT(node->initializer);
	}

#undef VISIT_NEXT
};

} // namespace igni::opt