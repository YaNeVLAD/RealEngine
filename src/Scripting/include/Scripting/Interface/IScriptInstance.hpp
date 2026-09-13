#pragma once

#include <Core/String.hpp>

#include <cstdint>

namespace re::scripting
{

class IScriptInstance
{
public:
	virtual ~IScriptInstance() = default;

	virtual bool InvokeMethod(
		String const& methodName,
		const void* args,
		std::uint32_t argsSize,
		void* outReturn,
		std::uint32_t outCapacity,
		std::uint32_t* outReturnSize) = 0;

	virtual void Release() {}

	virtual void OnCreate() = 0;

	virtual void OnUpdate(float deltaTime) = 0;
};

} // namespace re::scripting