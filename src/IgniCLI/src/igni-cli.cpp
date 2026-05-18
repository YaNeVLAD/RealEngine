#include <Core/FlatMap.hpp>
#include <IgniCLI/CLArguments.hpp>
#include <IgniCLI/Runners/DotNetRunner.hpp>
#include <IgniCLI/Runners/RvmRunner.hpp>
#include <IgniLang/Compiler/DotNetBackend.hpp>
#include <IgniLang/Compiler/Pipeline.hpp>
#include <IgniLang/Compiler/RvmBackend.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(const int argc, char** argv)
{
#ifdef RE_SYSTEM_WINDOWS
	SetConsoleOutputCP(CP_UTF8);
#endif

	using namespace std::literals;

	auto args = igni::cli::CLArguments(argc, argv);
	if (!args.Valid())
	{
		for (const auto& err : args.Errors())
		{
			std::cerr << err;
		}

		return 1;
	}

	const auto target = args.BuildTarget();
	static constexpr re::FlatMap<igni::BuildTarget, const char*, 2> TARGET_NAME_MAP = { {
		{ igni::BuildTarget::DotNet, "dotnet" },
		{ igni::BuildTarget::RVM, "rvm" },
	} };
	const auto targetId = TARGET_NAME_MAP[target];

	auto& sourceFiles = args.SourceFiles();
	if (const std::string stdlibPath = "assets/source/stdlib_"s + *targetId + ".igni"; std::filesystem::exists(stdlibPath))
	{
		sourceFiles.insert(sourceFiles.begin(), stdlibPath);
	}
	else
	{
		std::cerr << "[Warning] Standard library not found: " << stdlibPath << "\n";
	}

	try
	{
		std::unique_ptr<igni::compiler::IBackend> backend;
		if (target == igni::BuildTarget::RVM)
		{
			backend = std::make_unique<igni::compiler::RvmBackend>();
		}
		else if (target == igni::BuildTarget::DotNet)
		{
			backend = std::make_unique<igni::compiler::DotNetBackend>();
		}

		const igni::compiler::Pipeline pipeline("assets/igni_grammar.txt");
		const auto result = pipeline.Compile(sourceFiles, target, *backend);

		if (!result.success)
		{
			std::cerr << "[Compiler] Compilation failed due to errors.\n";
			return 1;
		}

		std::cout << "[Info] Code generated successfully.\n";
		std::cout << "------- GENERATED CODE -------\n"
				  << result.generatedCode
				  << "\n------------------------------\n";

		std::unique_ptr<igni::cli::IRunner> runner;
		if (target == igni::BuildTarget::RVM)
		{
			runner = std::make_unique<igni::cli::RvmRunner>();
		}
		else if (target == igni::BuildTarget::DotNet)
		{
			runner = std::make_unique<igni::cli::DotNetRunner>(args.BuildType());
		}

		return runner->Run(result.generatedCode);
	}
	catch (const std::exception& e)
	{
		std::cerr << "\n[Compiler Fatal Error] " << e.what() << "\n";
		return 1;
	}
}