#include <Core/flat_map.hpp>
#include <IgniCLI/BackendRegistry.hpp>
#include <IgniCLI/CLArguments.hpp>
#include <IgniCLI/Runners/DotNetRunner.hpp>
#include <IgniCLI/Runners/RvmRunner.hpp>
#include <IgniLang/Backend/TargetConfig.hpp>
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
	static constexpr auto TARGET_NAME_MAP = re::make_flat_map<enum igni::BuildTarget, const char*>({
		{ igni::BuildTarget::DotNet, "dotnet" },
		{ igni::BuildTarget::RVM, "rvm" },
	});
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
		re::String targetFlag = target == igni::BuildTarget::DotNet ? "--dotnet" : "--rvm";
		re::String backendExe;

		for (const auto& b : igni::backend::BackendRegistry::Load(argv[0]))
		{
			if (b.flag == targetFlag)
			{
				backendExe = b.executablePath;
				break;
			}
		}

		if (backendExe.Empty())
		{
			std::cerr << "[Error] Target backend executable not found for flag " << targetFlag << " in backends.json\n";
			return 1;
		}

		std::cout << "[Info] Fetching config from backend process: " << backendExe << "...\n";
		igni::TargetConfig targetConfig = igni::TargetConfig::LoadFromBackend(backendExe);

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
		const auto result = pipeline.Compile(sourceFiles, targetConfig, args.BuildType(), args.DisableDCE(), *backend);

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