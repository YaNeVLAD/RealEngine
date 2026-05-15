#pragma once

#include <IgniLang/Compiler/DotNetCodeGenerator.hpp>
#include <IgniLang/Compiler/IBackend.hpp>

#include <sstream>

namespace igni::compiler
{

class DotNetBackend : public IBackend
{
public:
	std::string Generate(
		const ast::Program* linkedProgram,
		const std::unordered_set<re::String>& globalNames,
		const std::unordered_map<re::String, re::String>& importAliases,
		const std::unordered_set<re::String>& externals,
		const sem::SemanticAnalyzer& semanticAnalyzer) override
	{
		std::stringstream out;

		DotNetCodeGenerator generator(out, semanticAnalyzer);
		generator.Generate(linkedProgram);

		return out.str();
	}
};

} // namespace igni::compiler