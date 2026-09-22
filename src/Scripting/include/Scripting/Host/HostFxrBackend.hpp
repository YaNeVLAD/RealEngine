#pragma once

#include <Scripting/Export.hpp>

#include <Core/LibraryLoader.hpp>
#include <Scripting/Host/IManagedHost.hpp>

#include <memory>

namespace re::scripting
{

class RE_SCRIPTING_API HostFxrBackend final : public IManagedHost
{
public:
	HostFxrBackend() = default;

	void Initialize(String const& runtimeConfig) override;
	void Shutdown() override;

	[[nodiscard]] void* LoadEntryPoint(
		String const& assemblyPath,
		String const& typeName,
		String const& methodName) const override;

private:
	std::unique_ptr<LibraryLoader> m_coreClrLoader;
	void* m_loadAssemblyFn = nullptr;
};

} // namespace re::scripting
