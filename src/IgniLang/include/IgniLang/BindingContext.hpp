#pragma once

#include <IgniLang/Export.hpp>

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <memory>
#include <unordered_map>

namespace igni
{

struct CallInfo
{
	std::shared_ptr<sem::FunctionType> target;
	re::String mangledTargetName;
	re::String mangledClassName;

	bool isConstructorCall = false;
	bool isSuperCall = false;
	bool isVarargCall = false;

	std::size_t varargCount = 0;
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