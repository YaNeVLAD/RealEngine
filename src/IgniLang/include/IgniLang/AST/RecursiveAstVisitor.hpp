#pragma once

#include <IgniLang/AST/AstNodes.hpp>

namespace igni::ast
{

class RecursiveAstVisitor : public BaseAstVisitor
{
public:
	void Visit(const SimpleTypeNode* node) override
	{
		Dispatch(node->typeArgs);
	}

	void Visit(const FunctionTypeNode* node) override
	{
		Dispatch(node->paramTypes);
		Dispatch(node->returnType);
	}

	void Visit(const BinaryExpr* node) override
	{
		Dispatch(node->left);
		Dispatch(node->right);
	}

	void Visit(const LiteralExpr*) override {}

	void Visit(const IdentifierExpr* node) override
	{
		Dispatch(node->typeArgs);
	}

	void Visit(const CallExpr* node) override
	{
		Dispatch(node->callee);
		Dispatch(node->arguments);
	}

	void Visit(const IndexExpr* node) override
	{
		Dispatch(node->array);
		Dispatch(node->index);
	}

	void Visit(const AssignExpr* node) override
	{
		Dispatch(node->target);
		Dispatch(node->value);
	}

	void Visit(const UnaryExpr* node) override
	{
		Dispatch(node->operand);
	}

	void Visit(const MemberAccessExpr* node) override
	{
		Dispatch(node->object);
		Dispatch(node->typeArgs);
	}

	void Visit(const AwaitExpr* node) override
	{
		Dispatch(node->expression);
	}

	void Visit(const LaunchExpr* node) override
	{
		Dispatch(node->callable);
	}

	void Visit(const ExprStmt* node) override
	{
		Dispatch(node->expr);
	}

	void Visit(const ReturnStmt* node) override
	{
		Dispatch(node->expr);
	}

	void Visit(const Block* node) override
	{
		Dispatch(node->statements);
	}

	void Visit(const IfStmt* node) override
	{
		Dispatch(node->condition);
		Dispatch(node->thenBranch);
		Dispatch(node->elseBranch);
	}

	void Visit(const WhileStmt* node) override
	{
		Dispatch(node->condition);
		Dispatch(node->body);
	}

	void Visit(const ForStmt* node) override
	{
		Dispatch(node->startExpr);
		Dispatch(node->endExpr);
		Dispatch(node->body);
	}

	void Visit(const AnnotationDecl* node) override
	{
		for (const auto& param : node->parameters)
		{
			Dispatch(param->type);
		}
	}

	void Visit(const ValDecl* node) override
	{
		Dispatch(node->initializer);
		Dispatch(node->type);
	}

	void Visit(const VarDecl* node) override
	{
		Dispatch(node->initializer);
		Dispatch(node->type);
	}

	void Visit(const FunDecl* node) override
	{
		for (const auto& param : node->parameters)
		{
			Dispatch(param->type);
		}
		for (const auto& tp : node->typeParams)
		{
			Dispatch(tp->boundType);
		}
		Dispatch(node->returnType);
		Dispatch(node->body);
	}

	void Visit(const ClassDecl* node) override
	{
		for (const auto& tp : node->typeParams)
		{
			Dispatch(tp->boundType);
		}
		if (node->baseClass)
		{
			Dispatch(node->baseClass->type);
			Dispatch(node->baseClass->arguments);
		}
		Dispatch(node->members);
	}

	void Visit(const ConstructorDecl* node) override
	{
		for (const auto& param : node->parameters)
		{
			Dispatch(param->type);
		}
		Dispatch(node->body);
	}

	void Visit(const DestructorDecl* node) override
	{
		Dispatch(node->body);
	}

	void Visit(const ImportDecl*) override {}

	void Visit(const TypeCastExpr* node) override
	{
		Dispatch(node->expr);
		Dispatch(node->targetType);
	}

	void Visit(const LambdaExpr* node) override
	{
		for (const auto& param : node->parameters)
		{
			Dispatch(param->type);
		}
		Dispatch(node->returnType);
		Dispatch(node->body);
	}

	void Visit(const Program* node) override
	{
		Dispatch(node->imports);
		Dispatch(node->statements);
	}

protected:
	void Dispatch(const Node* node)
	{
		if (node)
		{
			node->Accept(*this);
		}
	}

	template <typename T>
	void Dispatch(const std::unique_ptr<T>& node)
	{
		if (node)
		{
			node->Accept(*this);
		}
	}

	template <typename T>
	void Dispatch(const std::vector<std::unique_ptr<T>>& nodes)
	{
		for (const auto& node : nodes)
		{
			if (node)
			{
				node->Accept(*this);
			}
		}
	}
};

} // namespace igni::ast