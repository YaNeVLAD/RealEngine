#pragma once

#include <Core/String.hpp>
#include <Core/Utils.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace igni::ast
{

class IAstVisitor;

struct TypeNode;
using TypeEnv = std::unordered_map<re::String, const TypeNode*>;

struct Node
{
	virtual ~Node() = default;
	fsm::token<TokenType> token;
	virtual void Accept(IAstVisitor& visitor) const = 0;
};

template <typename TDerived, typename TBase = Node>
using AstVisitable = re::utils::Visitable<IAstVisitor, TDerived, TBase>;

enum class Visibility
{
	Public,
	Private,
	Internal,
};

struct Expr : Node
{
};
struct Statement : Node
{
};

struct AnnotationNode;

struct Decl : Statement
{
	Visibility visibility = Visibility::Public;
	std::vector<std::unique_ptr<AnnotationNode>> annotations;
};

struct TypeNode : Node
{
	bool isNullable = false;
};

} // namespace igni::ast