#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <algorithm>
#include <vector>

namespace igni::dotnet::detail
{

struct GeneratorState
{
	std::ostream& out;
	const sem::SemanticAnalyzer& analyzer;

	std::size_t labelCount = 0;
	bool isWritingGlobal = true;
	const ast::ClassDecl* currentClass = nullptr;

	std::vector<re::String> locals;
	std::vector<re::String> localTypes;
	std::vector<re::String> args;
	std::vector<re::String> argTypes;
	std::vector<re::String> externAssemblies;
	std::unordered_map<re::String, re::String> globalVars;

	GeneratorState(std::ostream& o, const sem::SemanticAnalyzer& a)
		: out(o)
		, analyzer(a)
	{
	}

	[[nodiscard]] int GetLocalIndex(const re::String& name) const
	{
		if (const auto it = std::ranges::find(locals, name); it != locals.end())
		{
			return static_cast<int>(std::distance(locals.begin(), it));
		}
		return -1;
	}

	[[nodiscard]] int GetArgIndex(const re::String& name) const
	{
		if (const auto it = std::ranges::find(args, name); it != args.end())
		{
			return static_cast<int>(std::distance(args.begin(), it));
		}
		return -1;
	}
};

} // namespace igni::dotnet::detail