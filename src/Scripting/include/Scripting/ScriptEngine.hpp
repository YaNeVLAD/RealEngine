#pragma once

#include <Core/FileSystem.hpp>
#include <Core/LibraryLoader.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/Chunk.hpp>
#include <RVM/VirtualMachine.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

namespace re::scripting
{

class ScriptEngine
{
public:
	void Init()
	{
		InitNativeLibrary();
		// BindTransformAPI(m_vm);
		// BindPhysicsAPI(m_vm); ...
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

		if (const auto result = m_vm.Interpret(m_mainChunk); result != re::rvm::InterpreterResult::Success)
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

private:
	rvm::VirtualMachine m_vm;
	rvm::Chunk m_mainChunk;

	std::unique_ptr<LibraryLoader> m_stdLibLoader;
};

} // namespace re::scripting