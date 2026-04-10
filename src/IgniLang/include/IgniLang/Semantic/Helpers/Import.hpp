#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Import
{

inline void Process(const ast::ImportDecl* node, SemanticContext& m_context)
{
	const re::String fullPath = node->path;

	if (node->isStar)
	{ // import module.*
		const Symbol* modSym = m_context.env.Resolve(fullPath);
		if (!modSym)
		{
			throw std::runtime_error("Semantic Error: Unknown module '" + fullPath + "' in import");
		}

		const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type);
		if (!modType)
		{
			throw std::runtime_error("Semantic Error: '" + fullPath + "' is not a module");
		}

		for (const auto& [exportName, exportType] : modType->exports)
		{
			m_context.env.Define(exportName, exportType, true);
			m_context.importAliases[exportName] = fullPath;
		}
	}
	else
	{ // import module.ident
		const std::size_t dotPos = fullPath.Find('.');
		if (dotPos == re::String::NPos)
		{
			return;
		}

		const auto modName = fullPath.Substring(0, dotPos);
		const auto memberName = fullPath.Substring(dotPos + 1);

		const Symbol* modSym = m_context.env.Resolve(modName);
		if (!modSym)
		{
			throw std::runtime_error("Semantic Error: Unknown module '" + modName + "' in import");
		}

		const auto modType = std::dynamic_pointer_cast<ModuleType>(modSym->type);
		if (!modType)
		{
			throw std::runtime_error("Semantic Error: '" + modName + "' is not a module");
		}

		if (const auto it = modType->exports.find(memberName); it != modType->exports.end())
		{
			m_context.env.Define(memberName, it->second, true);
			m_context.importAliases[memberName] = modName;
		}
		else
		{
			throw std::runtime_error("Semantic Error: Module '" + modName + "' has no export named '" + memberName + "'");
		}
	}
}

} // namespace igni::sem::Import