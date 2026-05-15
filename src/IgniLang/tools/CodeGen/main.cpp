#include <fstream>
#include <iostream>

#include "Builder.hpp"

#include "Emitters/BinaryOpsEmitter.hpp"
#include "Emitters/BuiltinsEmitter.hpp"
#include "Emitters/CastRulesEmitter.hpp"
#include "Emitters/DesugaringEmitter.hpp"
#include "Emitters/ImplicitPromotionsEmitter.hpp"
#include "Emitters/TargetConfigEmitter.hpp"
#include "Emitters/UnaryOpsEmitter.hpp"

#include "GeneratorEngine.hpp"

#include <nlohmann/json.hpp>

using namespace igni::codegen;

int main(const int argc, char** argv)
{
	if (argc < 4)
	{
		return 1;
	}

	std::string headerPath = argv[1];
	std::string sourcePath = argv[2];
	nlohmann::json combinedData;

	for (std::size_t i = 3; i < argc; ++i)
	{
		if (std::ifstream inputFile(argv[i]); inputFile.is_open())
		{
			combinedData.merge_patch(nlohmann::json::parse(inputFile));
		}
	}

	// --- HPP ---
	std::ofstream headerFile(headerPath);
	Builder<std::string> headerBuilder(headerFile);
	headerBuilder.Line("#pragma once");
	headerBuilder.Include("Core/String.hpp", true);
	headerBuilder.Include("unordered_map", true);
	headerBuilder.Include("unordered_set", true);
	headerBuilder.Include("IgniLang/BindingContext.hpp", true);
	headerBuilder.EmptyLine();

	auto nsHeader = headerBuilder.Namespace("igni::sem::generated");

	// --- CPP ---
	std::ofstream sourceFile(sourcePath);
	Builder<std::string> sourceBuilder(sourceFile);
	std::string headerName = headerPath.substr(headerPath.find_last_of("/\\") + 1);
	sourceBuilder.Include(headerName);
	sourceBuilder.Include("Core/HashedString.hpp", true);
	sourceBuilder.EmptyLine();

	auto nsSource = sourceBuilder.Namespace("igni::sem::generated");
	sourceBuilder.Line("using namespace re::literals;");
	sourceBuilder.EmptyLine();

	GeneratorEngine<
		BinaryOpsEmitter,
		UnaryOpsEmitter,
		CastRulesEmitter,
		ImplicitPromotionsEmitter,
		DesugaringEmitter,
		BuiltinsEmitter,
		TargetConfigEmitter>
		engine;

	engine.Run(combinedData, headerBuilder, sourceBuilder);

	return 0;
}