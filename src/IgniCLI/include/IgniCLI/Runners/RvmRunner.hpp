#pragma once

#include <IgniCLI/Runners/IRunner.hpp>

#include <Core/LibraryLoader.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/EventLoop.hpp>
#include <RVM/VirtualMachine.hpp>

#include <iostream>
#include <memory>

namespace igni::cli
{

using IgniPluginInitFn = void (*)(re::rvm::VirtualMachine*);

class RvmRunner final : public IRunner
{
public:
	int Run(const std::string& generatedCode) override
	{
		re::rvm::Chunk chunk;
		if (re::rvm::Assembler assembler; !assembler.Compile(generatedCode, chunk))
		{
			std::cerr << "[Error] RVM Assembler failed to compile bytecode.\n";
			return 1;
		}

		re::rvm::EventLoop eventLoop;
		re::rvm::VirtualMachine vm;

		vm.SetDelayHandler([&eventLoop](const re::rvm::CoroutinePtr& coro, const std::uint64_t ms) {
			eventLoop.Delay(coro, ms);
		});

		LoadStandardLibrary(vm);

		std::cout << "[Info] Running Virtual Machine...\n";
		std::cout << "=================================\n";

		auto result = vm.Interpret(chunk);
		if (result == re::rvm::InterpreterResult::Suspended)
		{
			result = eventLoop.Run(vm);
		}

		std::cout << "=================================\n";
		if (result != re::rvm::InterpreterResult::Success)
		{
			std::cerr << "[VM Error] Program exited with a runtime error.\n";
			return 1;
		}

		return 0;
	}

private:
	std::unique_ptr<re::LibraryLoader> m_stdlibPlugin;

	void LoadStandardLibrary(re::rvm::VirtualMachine& vm)
	{
		try
		{
			constexpr auto STD_LIB_NAME = "IgniStdLib.dll";
			m_stdlibPlugin = std::make_unique<re::LibraryLoader>(STD_LIB_NAME);

			const auto initFn = m_stdlibPlugin->GetSymbol<IgniPluginInitFn>("IgniPluginInit");
			initFn(&vm);

			std::cout << "[Info] Standard library plugin loaded successfully.\n";
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Warning] Could not load standard library: " << e.what() << "\n";
		}
	}
};

} // namespace igni::cli