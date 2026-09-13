#pragma once

#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
#include <Scripting/Interface/IScriptInstance.hpp>

namespace re
{

class DotNetScriptInstance final : public scripting::IScriptInstance
{
	void* m_GCHandle = nullptr;

public:
	explicit DotNetScriptInstance(void* GCHandle)
		: m_GCHandle(GCHandle)
	{
	}

	~DotNetScriptInstance() override
	{
		Release();
	}

	bool InvokeMethod(
		String const& methodName,
		const void* args,
		const std::uint32_t argsSize,
		void* outReturn,
		const std::uint32_t outCapacity,
		std::uint32_t* outReturnSize) override
	{
		if (!scripting::DotNetInterop::InvokeMethodByName || !m_GCHandle)
		{
			return false;
		}

		const std::string nameU8 = methodName.ToString();
		const auto [status, returnBytes] = scripting::DotNetInterop::InvokeMethodByName(
			m_GCHandle, nameU8.c_str(), args, argsSize, outReturn, outCapacity);

		if (outReturnSize)
		{
			*outReturnSize = static_cast<std::uint32_t>(returnBytes);
		}

		return status == 0;
	}

	void Release() override
	{
		if (!m_GCHandle)
		{
			return;
		}

		if (scripting::DotNetInterop::OnDestroy)
		{
			scripting::DotNetInterop::OnDestroy(m_GCHandle);
		}
		if (scripting::DotNetInterop::FreeInstance)
		{
			scripting::DotNetInterop::FreeInstance(m_GCHandle);
		}

		m_GCHandle = nullptr;
	}

	void OnCreate() override
	{
		if (scripting::DotNetInterop::InvokeOnCreate && m_GCHandle)
		{
			scripting::DotNetInterop::InvokeOnCreate(m_GCHandle);
		}
	}

	void OnUpdate(const float deltaTime) override
	{
		if (scripting::DotNetInterop::InvokeOnUpdate && m_GCHandle)
		{
			scripting::DotNetInterop::InvokeOnUpdate(m_GCHandle, deltaTime);
		}
	}
};

} // namespace re