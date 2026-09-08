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
		if (scripting::DotNetInterop::InvokeMethodByName)
		{
			const std::string nameU8 = methodName.ToString();
			scripting::DotNetInterop::InvokeMethodByName(m_GCHandle, nameU8.c_str());
		}
	}

	void GetFieldValue(String const& Field, void* outValue) override
	{
	}

	void OnCreate() override
	{
		if (scripting::DotNetInterop::InvokeOnCreate)
		{
			scripting::DotNetInterop::InvokeOnCreate(m_GCHandle);
		}
	}

	void OnUpdate(const float deltaTime) override
	{
		if (scripting::DotNetInterop::InvokeOnUpdate)
		{
			scripting::DotNetInterop::InvokeOnUpdate(m_GCHandle, deltaTime);
		}
	}
};

} // namespace re
