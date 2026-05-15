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

	if (argc < 2)
	{
		std::cerr << "Usage: igni-cli <file1.igni> <file2.igni> ...\n";
		return 1;
	}

	std::string targetId = "rvm";
	std::vector<std::string> sourceFiles;

	for (int i = 1; i < argc; ++i)
	{
		if (std::string arg = argv[i]; arg == "--dotnet")
		{
			targetId = "dotnet";
		}
		else if (arg == "--rvm")
		{
			targetId = "rvm";
		}
		else if (arg.length() > 0 && arg[0] == '-')
		{
			std::cerr << "[Warning] Unknown flag ignored: " << arg << "\n";
		}
		else
		{
			sourceFiles.emplace_back(arg);
		}
	}

	if (const std::string stdlibPath = "assets/source/stdlib_" + targetId + ".igni"; std::filesystem::exists(stdlibPath))
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
		if (targetId == "rvm")
		{
			backend = std::make_unique<igni::compiler::RvmBackend>();
		}
		else if (targetId == "dotnet")
		{
			backend = std::make_unique<igni::compiler::DotNetBackend>();
		}
		else
		{
			std::cerr << "Unknown target: " << targetId << "\n";
			return 1;
		}

		const igni::compiler::Pipeline pipeline("assets/igni_grammar.txt");
		const auto result = pipeline.Compile(sourceFiles, targetId, *backend);

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
		if (targetId == "rvm")
		{
			runner = std::make_unique<igni::cli::RvmRunner>();
		}
		else if (targetId == "dotnet")
		{
			runner = std::make_unique<igni::cli::DotNetRunner>();
		}

		return runner->Run(result.generatedCode);
	}
	catch (const std::exception& e)
	{
		std::cerr << "\n[Compiler Fatal Error] " << e.what() << "\n";
		return 1;
	}

	return 0;
}