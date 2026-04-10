#pragma once

#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Export
{

inline std::shared_ptr<SemanticType> Resolve(const std::shared_ptr<ModuleType>& modType, const re::String& member)
{
	if (const auto it = modType->exports.find(member); it != modType->exports.end())
	{
		return it->second;
	}

	throw std::runtime_error("Semantic Error: Module '" + modType->name + "' has no export named '" + member + "'");
}

} // namespace igni::sem::Export