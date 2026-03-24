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
	std::string Compile(const ast::Program* program)
	{
		std::vector<const ast::FunDecl*> flatFunctions;
		std::unordered_map<const ast::FunDecl*, std::vector<std::string>> functionUpvalues;
		std::unordered_map<const ast::FunDecl*, std::unordered_set<std::string>> functionBoxedVars;

		// Pass 1
		ScopeAnalyzer analyzer(flatFunctions, functionUpvalues, functionBoxedVars);
		analyzer.Analyze(program);

		// Pass 2
		std::stringstream out;
		CodeGenerator generator(out, flatFunctions, functionUpvalues, functionBoxedVars);
		generator.Generate(program);

		return out.str();
	}
};

} // namespace igni::compiler