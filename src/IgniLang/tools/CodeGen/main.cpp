#include <fstream>
#include <iostream>

#include "Builder.hpp"
#include "Emitters/BinaryOpsEmitter.hpp"
#include "Emitters/CastRulesEmitter.hpp"
#include "Emitters/UnaryOpsEmitter.hpp"
#include "GeneratorEngine.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace igni::codegen;

int main(const int argc, char** argv)
{
	if (argc < 3)
		return 1;

	std::ifstream inputFile(argv[1]);
	std::ofstream outputFile(argv[2]);
	json data = json::parse(inputFile);

	Builder<std::string> builder(outputFile);

	builder.Line("// ==========================================");
	builder.Line("// AUTO-GENERATED FILE. DO NOT EDIT.");
	builder.Line("// ==========================================");
	builder.EmptyLine();

	builder.Include("Core/String.hpp", true);
	builder.Include("Core/HashedString.hpp", true);
	builder.EmptyLine();

	auto nsScope = builder.Namespace("igni::sem::generated");
	builder.Line("using namespace re::literals;");
	builder.EmptyLine();

	GeneratorEngine<
		BinaryOpsEmitter,
		UnaryOpsEmitter,
		CastRulesEmitter>
		engine;

	engine.Run(data, builder);

	return 0;
}