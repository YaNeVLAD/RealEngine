#pragma once

#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
#include <Scripting/Interface/IscriptInstance.hpp>

namespace re
{

class DotNetScriptInstance : public scripting::IScriptInstance
{
	void* m_GCHandle = nullptr;

public:
	explicit DotNetScriptInstance(void* GCHandle)
		: m_GCHandle(GCHandle)
	{
	}

	void InvokeMethod(String const& methodName, void** args) override
	{
		// Обращение к кэшированному C# делегату для вызова метода у объекта по m_GCHandle
	}

	void GetFieldValue(String const& Field, void* outValue) override
	{
	}

	void OnCreate() const
	{
		if (scripting::DotNetInterop::InvokeOnCreate)
		{
			scripting::DotNetInterop::InvokeOnCreate(m_GCHandle);
		}
	}

	void OnUpdate(const float deltaTime) const
	{
		if (scripting::DotNetInterop::InvokeOnUpdate)
		{
			scripting::DotNetInterop::InvokeOnUpdate(m_GCHandle, deltaTime);
		}
	}
};

} // namespace re
