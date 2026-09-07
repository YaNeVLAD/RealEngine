#pragma once

#include <Scripting/Export.hpp>

#include <Core/LibraryLoader.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/Chunk.hpp>
#include <RVM/VirtualMachine.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace re::scripting
{

enum class RuntimeBackend : std::uint8_t
{
	RVM,
	CoreCLR,
};

class RE_SCRIPTING_API ScriptEngine
{
public:
	void Init(RuntimeBackend backend)
	{
		m_currentBackend = backend;

		switch (backend)
		{ // clang-format off
		case re::scripting::RuntimeBackend::RVM:     InitRVM();     break;
		case re::scripting::RuntimeBackend::CoreCLR: InitCoreCLR(); break;
		default: throw std::runtime_error("Unsupported scripting RuntimeBackend value: " + std::to_string((std::uint8_t)backend));
		} // clang-format on
	}

	bool LoadScript(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			std::cerr << "[ScriptEngine] Failed to open: " << filepath << "\n";
			return false;
		}

		std::stringstream ss;
		ss << file.rdbuf();
		std::string sourceCode = ss.str();

		if (rvm::Assembler assembler; !assembler.Compile(sourceCode, m_mainChunk))
		{
			std::cerr << "[ScriptEngine] Assembler error in: " << filepath << "\n";
			return false;
		}

		if (const auto result = m_vm.Interpret(m_mainChunk); result != rvm::InterpreterResult::Success)
		{
			std::cerr << "[ScriptEngine] VM failed to initialize chunk.\n";
			return false;
		}

		std::cout << "[ScriptEngine] Successfully loaded script: " << filepath << "\n";
		return true;
	}

	rvm::VirtualMachine* GetVM()
	{
		return &m_vm;
	}

private:
	void InitNativeLibrary()
	{
		using IgniPluginInitFn = void (*)(rvm::VirtualMachine*);

		try
		{
			constexpr auto STD_LIB_NAME = "IgniStdLib.dll";

			m_stdLibLoader = std::make_unique<LibraryLoader>(STD_LIB_NAME);

			const auto initFn = m_stdLibLoader->GetSymbol<IgniPluginInitFn>("IgniPluginInit");
			initFn(&m_vm);

			std::cout << "[Info] Standard library plugin loaded successfully.\n";
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Warning] Could not load standard library: " << e.what() << "\n";
		}
	}

	void InitRVM();

	void InitCoreCLR();

private:
	RuntimeBackend m_currentBackend;

	rvm::VirtualMachine m_vm;
	rvm::Chunk m_mainChunk;

	std::unique_ptr<LibraryLoader> m_stdLibLoader;

	std::unique_ptr<LibraryLoader> m_coreClrLoader;
	void* m_loadAssemblyFn = nullptr;
};

} // namespace re::scripting
