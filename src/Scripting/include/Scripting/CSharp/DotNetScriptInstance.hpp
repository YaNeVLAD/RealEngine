#pragma once

#include <Core/String.hpp>
#include <Scripting/Interface/IscriptInstance.hpp>

#include <cstdint>

namespace re
{

class DotNetScriptInstance : public scripting::IScriptInstance
{
	std::uint64_t m_GCHandle; // Handle для удержания C# объекта от сборщика мусора

public:
	void InvokeMethod(String const& methodName, void** args) override
	{
		// Обращение к кэшированному C# делегату для вызова метода у объекта по m_GCHandle
	}
};

} // namespace re
