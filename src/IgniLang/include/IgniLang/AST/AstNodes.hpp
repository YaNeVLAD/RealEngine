#pragma once

#include <Core/String.hpp>
#include <IgniLang/LexerFactory.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace igni::ast
{

#define AST_NODES(V)    \
	V(SimpleTypeNode)   \
	V(FunctionTypeNode) \
	V(BinaryExpr)       \
	V(LiteralExpr)      \
	V(IdentifierExpr)   \
	V(CallExpr)         \
	V(IndexExpr)        \
	V(AssignExpr)       \
	V(UnaryExpr)        \
	V(ExprStmt)         \
	V(ReturnStmt)       \
	V(Block)            \
	V(IfStmt)           \
	V(WhileStmt)        \
	V(ForStmt)          \
	V(VarDecl)          \
	V(ValDecl)          \
	V(FunDecl)          \
	V(ImportDecl)       \
	V(ClassDecl)        \
	V(MemberAccessExpr) \
	V(Program)

#define FORWARD_DECLARE_AST_NODE(Name) struct Name;
AST_NODES(FORWARD_DECLARE_AST_NODE)
#undef FORWARD_DECLARE_AST_NODE

class IAstVisitor
{
public:
	virtual ~IAstVisitor() = default;

#define DECLARE_VISITOR_METHOD(Name) virtual void Visit(const Name* node) = 0;
	AST_NODES(DECLARE_VISITOR_METHOD)
#undef DECLARE_VISITOR_METHOD
};

class BaseAstVisitor : public IAstVisitor
{
public:
	~BaseAstVisitor() override = default;

#define DECLARE_VISITOR_METHOD(Name) virtual void Visit(const Name*) override {};
	AST_NODES(DECLARE_VISITOR_METHOD)
#undef DECLARE_VISITOR_METHOD
};

// Базовый узел
struct Node
{
	virtual ~Node() = default;

	virtual void Print(int depth = 0) const = 0;
	virtual void Accept(IAstVisitor& visitor) const = 0;

protected:
	static void PrintIndent(const int depth)
	{
		std::cout << std::string(depth * 2, ' ');
	}
};

template <typename Derived, typename Base = Node>
struct Visitable : Base
{
	void Accept(IAstVisitor& visitor) const override
	{
		visitor.Visit(static_cast<const Derived*>(this));
	}
};

struct Expr : Node
{
};
struct Statement : Node
{
};
struct Decl : Statement
{
};
struct TypeNode : Node
{
	bool isNullable = false;
};

// ==========================================
// TYPES
// ==========================================
struct SimpleTypeNode final : Visitable<SimpleTypeNode, TypeNode>
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
			{
				arg->Print(depth + 1);
			}
		}
	}
};

struct FunctionTypeNode final : Visitable<FunctionTypeNode, TypeNode>
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
		{
			if (pt)
			{
				pt->Print(depth + 2);
			}
		}

		PrintIndent(depth + 1);
		std::cout << "Return:\n";
		if (returnType)
		{
			returnType->Print(depth + 2);
		}
	}
};

// ==========================================
// EXPRESSIONS
// ==========================================
struct BinaryExpr final : Visitable<BinaryExpr, Expr>
{
	std::unique_ptr<Expr> left;
	re::String op;
	std::unique_ptr<Expr> right;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "BinaryExpr [op: '" << op.ToString() << "']\n";
		if (left)
		{
			left->Print(depth + 1);
		}
		if (right)
		{
			right->Print(depth + 1);
		}
	}
};

struct LiteralExpr final : Visitable<LiteralExpr, Expr>
{
	fsm::token<TokenType> token;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "LiteralExpr [value: '" << token.lexeme << "']\n";
	}
};

struct IdentifierExpr final : Visitable<IdentifierExpr, Expr>
{
	re::String name;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "IdentifierExpr [name: '" << name.ToString() << "']\n";
	}
};

struct CallExpr final : Visitable<CallExpr, Expr>
{
	std::unique_ptr<Expr> callee;
	std::vector<std::unique_ptr<Expr>> arguments;

	bool isVarargCall = false;
	std::size_t varargCount = 0;

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
				{
					arg->Print(depth + 2);
				}
			}
		}
	}
};

struct IndexExpr final : Visitable<IndexExpr, Expr>
{
	std::unique_ptr<Expr> array;
	std::unique_ptr<Expr> index;

	void Print(int = 0) const override { /* ... */ }
};

struct AssignExpr final : Visitable<AssignExpr, Expr>
{
	std::unique_ptr<Expr> target;
	std::unique_ptr<Expr> value;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "AssignExpr\n";
		if (target)
		{
			target->Print(depth + 1);
		}
		if (value)
		{
			value->Print(depth + 1);
		}
	}
};

struct UnaryExpr final : Visitable<UnaryExpr, Expr>
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
		{
			operand->Print(depth + 1);
		}
	}
};

struct MemberAccessExpr final : Visitable<MemberAccessExpr, Expr>
{
	std::unique_ptr<Expr> object;
	re::String member;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "MemberAccessExpr [member: '" << member.ToString() << "']\n";
		if (object)
		{
			object->Print(depth + 1);
		}
	}
};

// ==========================================
// STATEMENTS
// ==========================================
struct ExprStmt final : Visitable<ExprStmt, Statement>
{
	std::unique_ptr<Expr> expr;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ExprStmt\n";
		if (expr)
		{
			expr->Print(depth + 1);
		}
	}
};

struct ReturnStmt final : Visitable<ReturnStmt, Statement>
{
	std::unique_ptr<Expr> expr; // Может быть nullptr

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ReturnStmt\n";
		if (expr)
		{
			expr->Print(depth + 1);
		}
	}
};

struct Block final : Visitable<Block, Statement>
{
	std::vector<std::unique_ptr<Statement>> statements;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "Block\n";
		for (const auto& stmt : statements)
		{
			if (stmt)
			{
				stmt->Print(depth + 1);
			}
		}
	}
};

struct IfStmt final : Visitable<IfStmt, Statement>
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
		{
			condition->Print(depth + 2);
		}

		PrintIndent(depth + 1);
		std::cout << "[Then]:\n";
		if (thenBranch)
		{
			thenBranch->Print(depth + 2);
		}

		if (elseBranch)
		{
			PrintIndent(depth + 1);
			std::cout << "[Else]:\n";
			elseBranch->Print(depth + 2);
		}
	}
};

struct WhileStmt final : Visitable<WhileStmt, Statement>
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
		{
			condition->Print(depth + 2);
		}

		PrintIndent(depth + 1);
		std::cout << "[Body]:\n";
		if (body)
		{
			body->Print(depth + 2);
		}
	}
};

struct ForStmt final : Visitable<ForStmt, Statement>
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
		{
			startExpr->Print(depth + 2);
		}

		PrintIndent(depth + 1);
		std::cout << "[End]:\n";
		if (endExpr)
		{
			endExpr->Print(depth + 2);
		}

		PrintIndent(depth + 1);
		std::cout << "[Body]:\n";
		if (body)
		{
			body->Print(depth + 2);
		}
	}
};

// ==========================================
// DECLARATIONS
// ==========================================
struct ValDecl final : Visitable<ValDecl, Decl>
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ValDecl [name: '" << name.ToString() << "']\n";
		if (initializer)
		{
			initializer->Print(depth + 1);
		}

		if (type)
		{
			type->Print(depth + 1);
		}
	}
};

struct VarDecl final : Visitable<VarDecl, Decl>
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "VarDecl [name: '" << name.ToString() << "']\n";
		if (initializer)
		{
			initializer->Print(depth + 1);
		}

		if (type)
		{
			type->Print(depth + 1);
		}
	}
};

struct FunDecl final : Visitable<FunDecl, Decl>
{
	struct Parameter
	{
		re::String name;
		std::unique_ptr<TypeNode> type;
	};

	re::String name;
	std::vector<Parameter> parameters;
	bool isVararg = false;
	bool isExternal = false;
	bool isExprBody = false;

	std::unique_ptr<Block> body;
	std::unique_ptr<TypeNode> returnType;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "FunDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
		std::cout << "Params: ";
		for (const auto& [paramName, type] : parameters)
		{
			std::cout << paramName.ToString() << " ";

			if (type)
			{
				type->Print(depth);
			}
		}
		std::cout << "\n";

		if (returnType)
		{
			returnType->Print(depth + 1);
		}

		if (body)
		{
			body->Print(depth + 1);
		}
	}
};

struct ClassDecl final : Visitable<ClassDecl, Decl>
{
	re::String name;

	std::vector<re::String> typeParams;

	std::vector<std::unique_ptr<Decl>> members;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ClassDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
	}
};

// ==========================================
// IMPORTS
// ==========================================
struct ImportDecl final : Visitable<ImportDecl>
{
	re::String path;
	bool isStar = false;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ImportDecl [path: '" << path.ToString() << "'"
				  << (isStar ? ".*" : "") << "]\n";
	}
};

// ==========================================
// PROGRAM ROOT
// ==========================================
struct Program final : Visitable<Program>
{
	re::String packageName;
	std::vector<std::unique_ptr<ImportDecl>> imports;
	std::vector<std::unique_ptr<Statement>> statements;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "Program [package: '" << packageName.ToString() << "']\n";
		for (const auto& import : imports)
		{
			if (import)
			{
				import->Print(depth + 1);
			}
		}

		for (const auto& stmt : statements)
		{
			if (stmt)
			{
				stmt->Print(depth + 1);
			}
		}
	}
};

} // namespace igni::ast