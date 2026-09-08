#pragma once

#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
#include <Scripting/CSharp/DotNetScriptInstance.hpp>
#include <Scripting/Interface/IScriptClass.hpp>
#include <Scripting/Interface/IScriptEngine.hpp>

#include <iostream>
#include <memory>

namespace re
{

class DotNetScriptEngine : public scripting::IScriptEngine
{
public:
	using IScriptEngine::LoadAssembly;

	void Init() override
	{
	}

	void Shutdown() override
	{
	}

	bool LoadAssembly(String const& filepath) override
	{
		if (!scripting::DotNetInterop::LoadUserAssembly)
		{
			return false;
		}

		const std::string pathU8 = filepath.ToString();
		const int result = scripting::DotNetInterop::LoadUserAssembly(pathU8.c_str());

		return result != 0;
	}

	scripting::IScriptClass* GetClass(String const& Namespace, String const& Class) override
	{
		return nullptr;
	}

	// Временный прямой метод для тестирования инстанцирования
	std::shared_ptr<DotNetScriptInstance> InstantiateTest(String const& fullClassName, std::uint64_t entityID)
	{
		if (!scripting::DotNetInterop::CreateInstance)
		{
			return nullptr;
		}

		const std::string nameU8 = fullClassName.ToString();
		void* handle = scripting::DotNetInterop::CreateInstance(nameU8.c_str(), entityID);

		if (!handle)
		{
			std::cerr << "[ScriptEngine] Failed to instantiate C# class: " << nameU8 << "\n";
			return nullptr;
		}

		return std::make_shared<DotNetScriptInstance>(handle);
	}
};

} // namespace re
