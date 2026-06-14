#pragma once

#include <IgniLang/Semantic/SemanticError.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Export
{

inline std::shared_ptr<SemanticType> Resolve(const std::shared_ptr<ModuleType>& modType, const ast::MemberAccessExpr* node)
{
	const auto& member = node->member;
	if (const auto it = modType->exports.find(member); it != modType->exports.end())
	{
		return it->second;
	}

	IGNI_SEM_ERR("Module '" + modType->name + "' has no export named '" + member + "'");
}

} // namespace igni::sem::Export