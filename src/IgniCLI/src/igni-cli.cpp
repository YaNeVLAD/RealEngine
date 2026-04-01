#include <Core/LibraryLoader.hpp>
#include <IgniCLI/Compiler.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/VirtualMachine.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using IgniPluginInitFn = void (*)(re::rvm::VirtualMachine*);

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

	std::vector<std::string> sourceFiles;

	if (std::filesystem::exists("assets/source/stdlib.igni"))
	{
		sourceFiles.emplace_back("assets/source/stdlib.igni");
	}

	for (int i = 1; i < argc; ++i)
	{
		sourceFiles.emplace_back(argv[i]);
	}

	try
	{
		const igni::Compiler pipeline("assets/lang_grammar.txt");
		const std::string assemblyCode = pipeline.CompileFiles(sourceFiles);

		std::cout << "[Info] Assembly generated successfully.\n";
		std::cout << "------- ASM -------\n"
				  << assemblyCode
				  << "\n-------------------\n";

		std::unique_ptr<re::LibraryLoader> stdlibPlugin = nullptr;
		re::rvm::VirtualMachine vm;
		try
		{
			constexpr auto STD_LIB_NAME = "IgniStdLib.dll";

			stdlibPlugin = std::make_unique<re::LibraryLoader>(STD_LIB_NAME);
			const auto initFn = stdlibPlugin->GetSymbol<IgniPluginInitFn>("IgniPluginInit");
			initFn(&vm);

			std::cout << "[Info] Standard library plugin loaded successfully.\n";
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Warning] Could not load standard library: " << e.what() << "\n";
		}

		re::rvm::Chunk chunk;
		if (re::rvm::Assembler assembler; !assembler.Compile(assemblyCode, chunk))
		{
			std::cerr << "[Error] Assembler failed to compile bytecode.\n";
			return 1;
		}

		std::cout << "[Info] Running Virtual Machine...\n";
		std::cout << "=================================\n";

		const auto result = vm.Interpret(chunk);

		std::cout << "=================================\n";
		if (result != re::rvm::InterpreterResult::Success)
		{
			std::cerr << "[VM Error] Program exited with a runtime error.\n";
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "\n[Compiler Fatal Error] " << e.what() << "\n";
		return 1;
	}

	return 0;
}