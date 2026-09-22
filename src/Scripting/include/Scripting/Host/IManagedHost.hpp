#pragma once

#include <Scripting/Export.hpp>

#include <Core/String.hpp>

namespace re::scripting
{

class RE_SCRIPTING_API IManagedHost
{
public:
	virtual ~IManagedHost() = default;

	virtual void Initialize(String const& runtimeConfig) = 0;

	virtual void Shutdown() = 0;

	[[nodiscard]] virtual void* LoadEntryPoint(
		String const& assemblyPath,
		String const& typeName,
		String const& methodName) const = 0;
};

} // namespace re::scripting
