#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Compiler/CodeGenerator.hpp>
#include <IgniLang/Compiler/ScopeAnalyzer.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace igni::compiler
{

class TextCompiler
{
public:
	std::string Compile(
		const ast::Program* program,
		const std::unordered_set<re::String>& globalNames,
		const std::unordered_map<re::String, re::String>& importAliases,
		const std::unordered_set<re::String>& externals,
		const sem::SemanticAnalyzer& semanticAnalyzer)
	{
		std::vector<const ast::FunDecl*> flatFunctions;
		std::unordered_map<const ast::FunDecl*, std::vector<re::String>> functionUpvalues;
		std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>> functionBoxedVars;

		// Pass 1
		ScopeAnalyzer analyzer(flatFunctions, functionUpvalues, functionBoxedVars, globalNames);
		analyzer.Analyze(program);

		// Pass 2
		std::stringstream out;
		CodeGenerator generator(out, flatFunctions, functionUpvalues, functionBoxedVars, importAliases, externals, semanticAnalyzer);
		generator.Generate(program);

		return out.str();
	}
};

} // namespace igni::compiler