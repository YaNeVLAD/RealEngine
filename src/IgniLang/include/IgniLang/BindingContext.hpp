#pragma once

#include <IgniLang/Export.hpp>

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace igni
{

enum class CallDispatchType
{
	Static,
	Virtual,
	Native,
	Indirect,
	TypeOf,
	TypeLiteral,
};

struct CallInfo
{
	std::shared_ptr<sem::FunctionType> target;
	re::String mangledTargetName;
	re::String mangledClassName;

	bool isConstructorCall = false;
	bool isSuperCall = false;
	bool isVarargCall = false;
	bool isImplicitThisCall = false;

	std::size_t varargCount = 0;

	CallDispatchType dispatchMode = CallDispatchType::Static;
	re::String asmLabel;
};

struct FunMeta
{
	re::String mangledName;
	bool isMethod = false;
	re::String parentClass;
};

struct LambdaMeta
{
	re::String mangledName;
};

class IGNI_API BindingContext
{
public:
	std::unordered_map<const ast::Expr*, std::shared_ptr<sem::SemanticType>> exprTypes;

	std::unordered_map<const ast::CallExpr*, CallInfo> callInfo;

	std::unordered_map<const ast::IdentifierExpr*, re::String> resolvedNames;
	std::unordered_map<const ast::MemberAccessExpr*, re::String> resolvedMembers;

	std::unordered_map<const ast::ClassDecl*, std::shared_ptr<sem::ClassType>> classTypes;
	std::unordered_map<const ast::FunDecl*, std::shared_ptr<sem::FunctionType>> funTypes;

	std::unordered_map<const ast::FunDecl*, re::String> mangledFunNames;

	std::unordered_map<const ast::Node*, FunMeta> funMeta;

	std::unordered_map<const ast::TypeCastExpr*, re::String> castTargets;

	std::unordered_map<const ast::LambdaExpr*, LambdaMeta> lambdaMeta;

	std::unordered_set<const ast::IdentifierExpr*> implicitThisNames;

	void SetExpressionType(const ast::Expr* expr, std::shared_ptr<sem::SemanticType> type)
	{
		exprTypes[expr] = std::move(type);
	}

	[[nodiscard]] std::shared_ptr<sem::SemanticType> GetExpressionType(const ast::Expr* expr) const
	{
		if (const auto it = exprTypes.find(expr); it != exprTypes.end())
		{
			return it->second;
		}

		return nullptr;
	}
};

} // namespace igni