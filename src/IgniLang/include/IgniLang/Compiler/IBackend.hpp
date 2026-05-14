#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>
#include <string>

namespace igni::compiler
{

class IBackend
{
public:
	virtual ~IBackend() = default;

	virtual std::string Generate(
		const ast::Program* linkedProgram,
		const std::unordered_set<re::String>& globalNames,
		const std::unordered_map<re::String, re::String>& importAliases,
		const std::unordered_set<re::String>& externals,
		const sem::SemanticAnalyzer& semanticAnalyzer) = 0;
};

} // namespace igni::compiler