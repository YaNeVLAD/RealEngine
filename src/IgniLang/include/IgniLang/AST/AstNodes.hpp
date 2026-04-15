#pragma once

#include <Core/String.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
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
	V(ConstructorDecl)  \
	V(DestructorDecl)   \
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

struct TypeNode;
using TypeEnv = std::unordered_map<re::String, const TypeNode*>;

struct Node
{
	virtual ~Node() = default;

	fsm::token<TokenType> token;

	virtual void Print(int depth = 0) const = 0;
	virtual void Accept(IAstVisitor& visitor) const = 0;

	virtual std::unique_ptr<Node> CloneNode(const TypeEnv* env = nullptr) const = 0;

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

enum class Visibility
{
	Public,
	Private,
	Internal,
};

struct Expr : Node
{
	virtual std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const = 0;
	std::unique_ptr<Node> CloneNode(const TypeEnv* env = nullptr) const override { return CloneExpr(env); }
};

struct Statement : Node
{
	virtual std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const = 0;
	std::unique_ptr<Node> CloneNode(const TypeEnv* env = nullptr) const override { return CloneStmt(env); }
};

struct Decl : Statement
{
	Visibility visibility = Visibility::Public;

	virtual std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const = 0;
	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override { return CloneDecl(env); }
};

struct TypeNode : Node
{
	bool isNullable = false;

	virtual std::unique_ptr<TypeNode> Clone(const TypeEnv* env = nullptr) const = 0;
	std::unique_ptr<Node> CloneNode(const TypeEnv* env = nullptr) const override { return Clone(env); }
};

// ==========================================
// TYPES
// ==========================================

struct SimpleTypeNode final : Visitable<SimpleTypeNode, TypeNode>
{
	re::String name;
	std::vector<std::unique_ptr<TypeNode>> typeArgs;

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

	[[nodiscard]] std::unique_ptr<TypeNode> Clone(const TypeEnv* env = nullptr) const override
	{
		if (env && env->contains(name))
		{
			auto substituted = env->at(name)->Clone(env);
			substituted->isNullable = this->isNullable || substituted->isNullable;
			return substituted;
		}

		auto clone = std::make_unique<SimpleTypeNode>();
		clone->name = name;
		clone->isNullable = isNullable;

		for (const auto& arg : typeArgs)
		{
			clone->typeArgs.emplace_back(arg->Clone(env));
		}

		return clone;
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

	[[nodiscard]] std::unique_ptr<TypeNode> Clone(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<FunctionTypeNode>();
		clone->isNullable = isNullable;

		for (const auto& p : paramTypes)
		{
			if (p)
			{
				clone->paramTypes.emplace_back(p->Clone(env));
			}
		}
		clone->returnType = returnType ? returnType->Clone(env) : nullptr;

		return clone;
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

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<BinaryExpr>();
		clone->op = op;
		if (left)
		{
			clone->left = left->CloneExpr(env);
		}
		if (right)
		{
			clone->right = right->CloneExpr(env);
		}

		return clone;
	}
};

struct LiteralExpr final : Visitable<LiteralExpr, Expr>
{
	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "LiteralExpr [value: '" << token.lexeme << "']\n";
	}

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* = nullptr) const override
	{
		return std::make_unique<LiteralExpr>(*this);
	}
};

struct IdentifierExpr final : Visitable<IdentifierExpr, Expr>
{
	re::String name;
	std::vector<std::unique_ptr<TypeNode>> typeArgs;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "IdentifierExpr [name: '" << name.ToString() << "']\n";
	}

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<IdentifierExpr>();
		clone->name = name;
		for (const auto& t : typeArgs)
		{
			clone->typeArgs.push_back(t ? t->Clone(env) : nullptr);
		}

		return clone;
	}
};

struct CallExpr final : Visitable<CallExpr, Expr>
{
	std::unique_ptr<Expr> callee;
	std::vector<std::unique_ptr<Expr>> arguments;

	bool isVarargCall = false;
	std::size_t varargCount = 0;

	bool isConstructorCall = false;
	bool isSuperCall = false;

	re::String staticMethodTarget;

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

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<CallExpr>();
		if (callee)
		{
			clone->callee = callee->CloneExpr(env);
		}
		for (const auto& arg : arguments)
		{
			clone->arguments.push_back(arg ? arg->CloneExpr(env) : nullptr);
		}
		clone->isVarargCall = isVarargCall;
		clone->varargCount = varargCount;
		clone->isConstructorCall = isConstructorCall;
		clone->isSuperCall = isSuperCall;
		clone->staticMethodTarget = staticMethodTarget;

		return clone;
	}
};

struct IndexExpr final : Visitable<IndexExpr, Expr>
{
	std::unique_ptr<Expr> array;
	std::unique_ptr<Expr> index;

	void Print(int = 0) const override { /* ... */ }

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<IndexExpr>();
		if (array)
		{
			clone->array = array->CloneExpr(env);
		}
		if (index)
		{
			clone->index = index->CloneExpr(env);
		}

		return clone;
	}
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

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<AssignExpr>();
		if (target)
		{
			clone->target = target->CloneExpr(env);
		}
		if (value)
		{
			clone->value = value->CloneExpr(env);
		}

		return clone;
	}
};

struct UnaryExpr final : Visitable<UnaryExpr, Expr>
{
	re::String op;
	std::unique_ptr<Expr> operand;
	bool isPostfix = false; // true - (x++), false - (++x) and (-x)

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

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<UnaryExpr>();
		clone->op = op;
		clone->isPostfix = isPostfix;
		if (operand)
		{
			clone->operand = operand->CloneExpr(env);
		}
		return clone;
	}
};

struct MemberAccessExpr final : Visitable<MemberAccessExpr, Expr>
{
	std::unique_ptr<Expr> object;
	re::String member;
	std::vector<std::unique_ptr<TypeNode>> typeArgs;

	bool isSafe = true;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "MemberAccessExpr [member: '" << member.ToString() << "']\n";
		if (object)
		{
			object->Print(depth + 1);
		}
	}

	std::unique_ptr<Expr> CloneExpr(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<MemberAccessExpr>();
		clone->member = member;
		if (object)
		{
			clone->object = object->CloneExpr(env);
		}
		for (const auto& t : typeArgs)
		{
			clone->typeArgs.push_back(t ? t->Clone(env) : nullptr);
		}

		return clone;
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

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ExprStmt>();
		if (expr)
		{
			clone->expr = expr->CloneExpr(env);
		}

		return clone;
	}
};

struct ReturnStmt final : Visitable<ReturnStmt, Statement>
{
	std::unique_ptr<Expr> expr;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ReturnStmt\n";
		if (expr)
		{
			expr->Print(depth + 1);
		}
	}

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ReturnStmt>();
		if (expr)
		{
			clone->expr = expr->CloneExpr(env);
		}

		return clone;
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

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<Block>();
		for (const auto& s : statements)
		{
			clone->statements.push_back(s ? s->CloneStmt(env) : nullptr);
		}

		return clone;
	}
};

struct IfStmt final : Visitable<IfStmt, Statement>
{
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Block> thenBranch;
	std::unique_ptr<Statement> elseBranch;

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

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<IfStmt>();
		if (condition)
		{
			clone->condition = condition->CloneExpr(env);
		}
		if (thenBranch)
		{
			clone->thenBranch.reset(dynamic_cast<Block*>(thenBranch->CloneStmt(env).release()));
		}
		if (elseBranch)
		{
			clone->elseBranch = elseBranch->CloneStmt(env);
		}

		return clone;
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

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<WhileStmt>();
		if (condition)
		{
			clone->condition = condition->CloneExpr(env);
		}
		if (body)
		{
			clone->body.reset(dynamic_cast<Block*>(body->CloneStmt(env).release()));
		}

		return clone;
	}
};

struct ForStmt final : Visitable<ForStmt, Statement>
{
	re::String iteratorName;
	std::unique_ptr<Expr> startExpr;
	std::unique_ptr<Expr> endExpr;
	std::unique_ptr<Block> body;

	bool isForEach = false;

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

	std::unique_ptr<Statement> CloneStmt(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ForStmt>();
		clone->iteratorName = iteratorName;
		clone->isForEach = isForEach;
		if (startExpr)
		{
			clone->startExpr = startExpr->CloneExpr(env);
		}
		if (endExpr)
		{
			clone->endExpr = endExpr->CloneExpr(env);
		}
		if (body)
		{
			clone->body.reset(dynamic_cast<Block*>(body->CloneStmt(env).release()));
		}
		return clone;
	}
};

// ==========================================
// DECLARATIONS
// ==========================================

struct GenericTypeParam
{
	re::String name;
	std::unique_ptr<TypeNode> boundType;

	GenericTypeParam() = default;

	GenericTypeParam(GenericTypeParam&&) noexcept = default;
	GenericTypeParam& operator=(GenericTypeParam&&) noexcept = default;

	GenericTypeParam(const GenericTypeParam& other)
		: name(other.name)
		, boundType(other.boundType ? other.boundType->Clone() : nullptr)
	{
	}

	GenericTypeParam& operator=(const GenericTypeParam& other)
	{
		if (this != &other)
		{
			name = other.name;
			boundType = other.boundType ? other.boundType->Clone() : nullptr;
		}

		return *this;
	}
};

struct ValDecl final : Visitable<ValDecl, Decl>
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;
	bool isExternal = false;

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

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ValDecl>();
		clone->visibility = visibility;
		clone->name = name;
		if (initializer)
		{
			clone->initializer = initializer->CloneExpr(env);
		}
		if (type)
		{
			clone->type = type->Clone(env);
		}

		return clone;
	}
};

struct VarDecl final : Visitable<VarDecl, Decl>
{
	re::String name;
	std::unique_ptr<Expr> initializer;
	std::unique_ptr<TypeNode> type;
	bool isExternal = false;

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

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<VarDecl>();
		clone->visibility = visibility;
		clone->name = name;
		if (initializer)
		{
			clone->initializer = initializer->CloneExpr(env);
		}
		if (type)
		{
			clone->type = type->Clone(env);
		}

		return clone;
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
	bool isOverride = false;

	std::unique_ptr<Block> body;
	std::unique_ptr<TypeNode> returnType;

	std::vector<GenericTypeParam> typeParams;

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

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<FunDecl>();
		clone->visibility = visibility;
		clone->name = name;
		for (const auto& p : parameters)
		{
			Parameter newP;
			newP.name = p.name;
			if (p.type)
			{
				newP.type = p.type->Clone(env);
			}
			clone->parameters.push_back(std::move(newP));
		}

		clone->isVararg = isVararg;
		clone->isExternal = isExternal;
		clone->isExprBody = isExprBody;
		clone->isOverride = isOverride;

		if (body)
		{
			clone->body.reset(dynamic_cast<Block*>(body->CloneStmt(env).release()));
		}
		if (returnType)
		{
			clone->returnType = returnType->Clone(env);
		}
		clone->typeParams = typeParams;

		return clone;
	}
};

struct BaseClassInit
{
	std::unique_ptr<TypeNode> type;
	std::vector<std::unique_ptr<Expr>> arguments;
};

struct ClassDecl final : Visitable<ClassDecl, Decl>
{
	re::String name;
	bool isExternal = false;

	std::vector<GenericTypeParam> typeParams;

	std::unique_ptr<BaseClassInit> baseClass;

	std::vector<std::unique_ptr<Decl>> members;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ClassDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
	}

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ClassDecl>();
		clone->visibility = visibility;
		clone->name = name;
		clone->isExternal = isExternal;
		clone->typeParams = typeParams;

		if (baseClass)
		{
			clone->baseClass = std::make_unique<BaseClassInit>();
			if (baseClass->type)
			{
				clone->baseClass->type = baseClass->type->Clone(env);
			}

			for (const auto& arg : baseClass->arguments)
			{
				if (arg)
				{
					clone->baseClass->arguments.emplace_back(std::unique_ptr<Expr>(arg->CloneExpr(env).release()));
				}
			}
		}

		for (const auto& member : members)
		{
			if (member)
			{
				clone->members.emplace_back(std::unique_ptr<Decl>(member->CloneDecl(env).release()));
			}
		}

		return clone;
	}
};

struct ConstructorDecl final : Visitable<ConstructorDecl, Decl>
{
	re::String name;
	std::vector<FunDecl::Parameter> parameters;
	std::unique_ptr<Block> body;

	bool isVararg = false;
	bool isExternal = false;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "ConstructorDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
		std::cout << "Parameters: ";
		for (const auto& [paramName, type] : parameters)
		{
			std::cout << paramName.ToString() << " ";
			if (type)
			{
				type->Print(depth);
			}
		}
	}

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<ConstructorDecl>();
		clone->visibility = visibility;
		clone->name = name;
		clone->isExternal = isExternal;
		clone->isVararg = isVararg;
		for (const auto& [pName, type] : parameters)
		{
			FunDecl::Parameter newP;
			newP.name = pName;
			if (type)
			{
				newP.type = type->Clone(env);
			}
			clone->parameters.push_back(std::move(newP));
		}
		if (body)
		{
			clone->body.reset(dynamic_cast<Block*>(body->CloneStmt(env).release()));
		}

		return clone;
	}
};

struct DestructorDecl final : Visitable<DestructorDecl, Decl>
{
	re::String name;
	std::unique_ptr<Block> body;

	bool isExternal = false;

	void Print(int depth = 0) const override
	{
		PrintIndent(depth);
		std::cout << "DestructorDecl [name: '" << name.ToString() << "']\n";
		PrintIndent(depth + 1);
	}

	std::unique_ptr<Decl> CloneDecl(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<DestructorDecl>();
		clone->visibility = visibility;
		clone->name = name;
		clone->isExternal = isExternal;
		if (body)
		{
			clone->body.reset(static_cast<Block*>(body->CloneStmt(env).release()));
		}

		return clone;
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

	std::unique_ptr<Node> CloneNode(const TypeEnv* = nullptr) const override
	{
		auto clone = std::make_unique<ImportDecl>();
		clone->path = path;
		clone->isStar = isStar;

		return clone;
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

	std::unique_ptr<Node> CloneNode(const TypeEnv* env = nullptr) const override
	{
		auto clone = std::make_unique<Program>();
		clone->packageName = packageName;

		for (const auto& imp : imports)
		{
			if (imp)
			{
				clone->imports.push_back(std::unique_ptr<ImportDecl>(static_cast<ImportDecl*>(imp->CloneNode(env).release())));
			}
		}

		for (const auto& stmt : statements)
		{
			if (stmt)
			{
				clone->statements.push_back(stmt->CloneStmt(env));
			}
		}

		return clone;
	}
};

} // namespace igni::ast