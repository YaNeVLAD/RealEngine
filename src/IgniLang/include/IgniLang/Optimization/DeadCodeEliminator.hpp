#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace igni::opt
{

class DeadCodeEliminator final : public ast::BaseAstVisitor
{
public:
	void Eliminate(ast::Program* program, const BindingContext& bindings)
	{
		m_bindings = &bindings;
		m_reachableNames.clear();
		m_worklist.clear();
		m_allBodies.clear();
		m_dynamicMethods.clear();

		for (const auto& stmt : program->statements)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				if (fun->typeParams.empty() && !fun->isExternal)
				{
					re::String mangledName = GetMangledName(fun);
					m_allBodies[mangledName] = fun->body.get();
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
							re::String mangledName = GetMangledName(mFun);
							m_allBodies[mangledName] = mFun->body.get();

							m_dynamicMethods[mFun->name].push_back(mangledName);
						}
						else if (const auto mCtor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
						{
							m_allBodies[GetMangledName(mCtor)] = mCtor->body.get();
						}
						else if (const auto mDtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
						{
							m_allBodies[GetMangledName(mDtor)] = mDtor->body.get();
						}
					}
				}
			}
		}

		MarkReached("main");

		for (const auto& stmt : program->statements)
		{
			if (!dynamic_cast<const ast::FunDecl*>(stmt.get()) && !dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				stmt->Accept(*this);
			}
		}

		while (!m_worklist.empty())
		{
			const re::String current = m_worklist.back();
			m_worklist.pop_back();

			if (m_allBodies.contains(current) && m_allBodies.at(current))
			{
				m_allBodies.at(current)->Accept(*this);
			}
		}

		std::erase_if(program->statements, [&](const std::unique_ptr<ast::Statement>& stmt) {
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				return !m_reachableNames.contains(GetMangledName(fun));
			}

			return false;
		});

		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<ast::ClassDecl*>(stmt.get()))
			{
				if (!classDecl->typeParams.empty())
				{
					continue;
				}

				std::erase_if(classDecl->members, [&](const std::unique_ptr<ast::Decl>& member) {
					if (const auto mFun = dynamic_cast<const ast::FunDecl*>(member.get()))
					{
						return !m_reachableNames.contains(GetMangledName(mFun));
					}
					if (const auto mCtor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						return !m_reachableNames.contains(GetMangledName(mCtor));
					}
					if (const auto mDtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
					{
						return !m_reachableNames.contains(GetMangledName(mDtor));
					}

					return false;
				});
			}
		}
	}

private:
	const BindingContext* m_bindings = nullptr;
	std::unordered_set<re::String> m_reachableNames;
	std::vector<re::String> m_worklist;
	std::unordered_map<re::String, const ast::Block*> m_allBodies;

	std::unordered_map<re::String, std::vector<re::String>> m_dynamicMethods;

	re::String GetMangledName(const ast::Decl* decl) const
	{
		if (m_bindings && m_bindings->funMeta.contains(decl))
		{
			return m_bindings->funMeta.at(decl).mangledName;
		}

		if (const auto f = dynamic_cast<const ast::FunDecl*>(decl))
		{
			return f->name;
		}
		if (const auto c = dynamic_cast<const ast::ConstructorDecl*>(decl))
		{
			return c->name;
		}
		if (const auto d = dynamic_cast<const ast::DestructorDecl*>(decl))
		{
			return d->name;
		}

		return {};
	}

	void MarkReached(const re::String& name)
	{
		if (!name.Empty() && !m_reachableNames.contains(name))
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
		if (m_bindings && m_bindings->callInfo.contains(node))
		{
			const auto& callInfo = m_bindings->callInfo.at(node);

			if (callInfo.dispatchMode == CallDispatchType::Static)
			{
				MarkReached(callInfo.asmLabel);
			}
			else if (callInfo.dispatchMode == CallDispatchType::Virtual)
			{
				if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
				{
					if (m_dynamicMethods.contains(memAccess->member))
					{
						for (const auto& mangled : m_dynamicMethods.at(memAccess->member))
						{
							MarkReached(mangled);
						}
					}
				}
			}

			if (callInfo.isConstructorCall)
			{
				MarkReached(callInfo.mangledClassName + "_destructor");
			}
		}

		VISIT_NEXT(node->callee);
		for (const auto& arg : node->arguments)
		{
			VISIT_NEXT(arg);
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		if (m_bindings && m_bindings->resolvedNames.contains(node))
		{
			MarkReached(m_bindings->resolvedNames.at(node)); // Захват замыканий
		}
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		if (m_bindings && m_bindings->resolvedMembers.contains(node))
		{
			MarkReached(m_bindings->resolvedMembers.at(node)); // Референсы на методы
		}
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

	void Visit(const ast::AwaitExpr* node) override
	{
		VISIT_NEXT(node->expression);
	}

	void Visit(const ast::LaunchExpr* node) override
	{
		VISIT_NEXT(node->callable);
	}

	void Visit(const ast::TypeCastExpr* node) override
	{
		VISIT_NEXT(node->expr);
	}

	void Visit(const ast::LambdaExpr* node) override
	{
		VISIT_NEXT(node->body);
	}

#undef VISIT_NEXT
};

} // namespace igni::opt