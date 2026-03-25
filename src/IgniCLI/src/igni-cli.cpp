#include <IgniCLI/Compiler.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/VirtualMachine.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

void InitIgniStdLib(re::rvm::VirtualMachine& vm)
{
	using namespace re::rvm;
	using namespace re::literals;

	vm.RegisterNative("print", [](std::vector<Value> const& args) -> Value {
		std::cout << ">>> [SCRIPT]: ";

		if (!args.empty() && std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			for (const auto arr = std::get<ArrayInstancePtr>(args[0]); const auto& arg : arr->elements)
			{
				std::cout << arg << " ";
			}
		}
		else
		{ // Fallback
			for (const auto& arg : args)
			{
				std::cout << arg << " ";
			}
		}

		std::cout << "\n";
		return Null;
	});

	auto typeArray = std::make_shared<TypeInfo>("Array");
	auto getMethod = std::make_shared<NativeObject>();
	getMethod->name = "get";
	getMethod->argCount = 1;
	getMethod->function = [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		const auto index = std::get<Int>(args[1]);
		return arr->elements[index];
	};
	typeArray->methods["get"_hs] = getMethod;

	auto setMethod = std::make_shared<NativeObject>();
	setMethod->name = "set";
	setMethod->argCount = 2;
	setMethod->function = [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		const auto index = std::get<Int>(args[1]);
		arr->elements[index] = args[2];
		return Null;
	};
	typeArray->methods["set"_hs] = setMethod;

	vm.RegisterType(typeArray);
	vm.RegisterNative("make_array", [typeArray](const std::vector<Value>& args) -> Value {
		auto arr = std::make_shared<ArrayInstance>();
		arr->typeInfo = typeArray;
		arr->elements.resize(std::get<Int>(args[0]));
		return arr;
	});
}

int main(const int argc, char** argv)
{
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

		re::rvm::VirtualMachine vm;
		InitIgniStdLib(vm);

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