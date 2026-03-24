#pragma once

#include <Core/String.hpp>
#include <IgniLang/LexerFactory.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace igni::ast
{

// Базовый узел
struct Node
{
	virtual ~Node() = default;

	virtual void Print(int depth = 0) const = 0;

protected:
	void PrintIndent(int depth) const
	{
		std::cout << std::string(depth * 2, ' ');
	}
};

struct TypeNode : Node
{
	bool isNullable = false;
};

struct SimpleTypeNode final : TypeNode
{
	re::String name; // Int, Float, Array
	std::vector<std::unique_ptr<TypeNode>> typeArgs; // <T1, T2>

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "SimpleType [name: '" << name.ToString() << "'"
				  << (isNullable ? ", nullable" : "") << "]\n";
		for (const auto& arg : typeArgs)
		{
			if (arg)
				arg->Print(depth + 1);
		}
	}
};

struct FunctionTypeNode final : TypeNode
{
	std::vector<std::unique_ptr<TypeNode>> paramTypes;
	std::unique_ptr<TypeNode> returnType;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "FunctionType" << (isNullable ? " [nullable]" : "") << "\n";

		PrintIndent(depth + 1);
		std::cout << "Params:\n";
		for (const auto& pt : paramTypes)
			if (pt)
				pt->Print(depth + 2);

		PrintIndent(depth + 1);
		std::cout << "Return:\n";
		if (returnType)
			returnType->Print(depth + 2);
	}
};

// --- Обновляем параметры функции ---
struct Parameter
{
	re::String name;
	std::unique_ptr<TypeNode> type;
};

// --- Выражения ---
struct Expr : Node
{
};

struct BinaryExpr final : Expr
{
	std::unique_ptr<Expr> left;
	re::String op;
	std::unique_ptr<Expr> right;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "BinaryExpr [op: '" << op.ToString() << "']\n";
		if (left)
			left->Print(depth + 1);
		if (right)
			right->Print(depth + 1);
	}
};

struct UnaryExpr final : Expr
{
	re::String op;
	std::unique_ptr<Expr> operand;
	bool isPostfix = false; // true для (x++), false для (++x) и (-x)

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "UnaryExpr [op: '" << op.ToString() << "'"
				  << (isPostfix ? ", postfix" : ", prefix") << "]\n";
		if (operand)
			operand->Print(depth + 1);
	}
};

struct LiteralExpr final : Expr
{
	fsm::token<TokenType> token;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "LiteralExpr [value: '" << token.lexeme << "']\n";
	}
};

struct IdentifierExpr final : Expr
{
	re::String name;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "IdentifierExpr [name: '" << name.ToString() << "']\n";
	}
};

struct CallExpr final : Expr
{
	std::unique_ptr<Expr> callee;
	std::vector<std::unique_ptr<Expr>> arguments;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "CallExpr\n";
		if (callee)
		{
			PrintIndent(depth + 1);
			std::cout << "[Callee]:\n";
			callee->Print(depth + 2);
		}
		if (!arguments.empty())
		{
			PrintIndent(depth + 1);
			std::cout << "[Arguments]:\n";
			for (const auto& arg : arguments)
			{
				if (arg)
					arg->Print(depth + 2);
			}
		}
	}
};

struct AssignExpr final : Expr
{
	std::unique_ptr<Expr> target; // Кому присваиваем (IdentifierExpr или IndexExpr)
	std::unique_ptr<Expr> value; // Что присваиваем

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "AssignExpr\n";
		if (target)
			target->Print(depth + 1);
		if (value)
			value->Print(depth + 1);
	}
};

// --- Инструкции (Statements) ---
struct Statement : Node
{
};

struct ExprStmt final : Statement
{
	std::unique_ptr<Expr> expr;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ExprStmt\n";
		if (expr)
			expr->Print(depth + 1);
	}
};

struct ReturnStmt final : Statement
{
	std::unique_ptr<Expr> expr; // Может быть nullptr

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ReturnStmt\n";
		if (expr)
			expr->Print(depth + 1);
	}
};

struct Block final : Statement
{
	std::vector<std::unique_ptr<Statement>> statements;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "Block\n";
		for (const auto& stmt : statements)
		{
			if (stmt)
				stmt->Print(depth + 1);
		}
	}
};

struct IfStmt final : Statement
{
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Block> thenBranch;
	std::unique_ptr<Statement> elseBranch; // Может быть Block или другой IfStmt, или nullptr

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "IfStmt\n";

		PrintIndent(depth + 1);
		std::cout << "[Condition]:\n";
		if (condition)
			condition->Print(depth + 2);

		PrintIndent(depth + 1);
		std::cout << "[Then]:\n";
		if (thenBranch)
			thenBranch->Print(depth + 2);

		if (elseBranch)
		{
			PrintIndent(depth + 1);
			std::cout << "[Else]:\n";
			elseBranch->Print(depth + 2);
		}
	}
};

struct WhileStmt final : Statement
{
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Block> body;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "WhileStmt\n";

		PrintIndent(depth + 1);
		std::cout << "[Condition]:\n";
		if (condition)
			condition->Print(depth + 2);

		PrintIndent(depth + 1);
		std::cout << "[Body]:\n";
		if (body)
			body->Print(depth + 2);
	}
};

struct ForStmt final : Statement
{
	re::String iteratorName;
	std::unique_ptr<Expr> startExpr;
	std::unique_ptr<Expr> endExpr;
	std::unique_ptr<Block> body;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ForStmt [iterator: '" << iteratorName.ToString() << "']\n";

		PrintIndent(depth + 1);
		std::cout << "[Start]:\n";
		if (startExpr)
			startExpr->Print(depth + 2);

		PrintIndent(depth + 1);
		std::cout << "[End]:\n";
		if (endExpr)
			endExpr->Print(depth + 2);

		PrintIndent(depth + 1);
		std::cout << "[Body]:\n";
		if (body)
			body->Print(depth + 2);
	}
};

// --- Объявления ---
struct Decl : Statement
{
};

struct ValDecl final : Decl
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ValDecl [name: '" << name.ToString() << "']\n";
		if (initializer)
			initializer->Print(depth + 1);

		if (type)
			type->Print(depth + 1);
	}
};

struct VarDecl final : Decl
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "VarDecl [name: '" << name.ToString() << "']\n";
		if (initializer)
			initializer->Print(depth + 1);

		if (type)
			type->Print(depth + 1);
	}
};

struct FunDecl final : Decl
{
	re::String name;
	std::vector<Parameter> parameters;
	std::unique_ptr<Block> body;
	std::unique_ptr<TypeNode> returnType;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "FunDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
		std::cout << "Params: ";
		for (const auto& [name, type] : parameters)
		{
			std::cout << name.ToString() << " ";

			if (type)
				type->Print(depth);
		}
		std::cout << "\n";

		if (returnType)
			returnType->Print(depth + 1);

		if (body)
			body->Print(depth + 1);
	}
};

// --- Корень программы ---
struct Program final : Node
{
	re::String packageName;
	std::vector<std::unique_ptr<Statement>> statements;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "Program [package: '" << packageName.ToString() << "']\n";
		for (const auto& stmt : statements)
		{
			if (stmt)
				stmt->Print(depth + 1);
		}
	}
};

} // namespace igni::ast