#include <Scripting/Host/HostFxrBackend.hpp>

#include <Core/Logger.hpp>

#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>

#include <stdexcept>
#include <string>

namespace re::scripting
{

namespace
{

using NativeString = std::basic_string<char_t>;

NativeString ToNativeString(String const& text)
{
#if defined(RE_SYSTEM_WINDOWS)
	return text.ToWString();
#else
	return text.ToString();
#endif
}

} // namespace

void HostFxrBackend::Initialize(String const& runtimeConfig)
{
	char_t buffer[1024];
	size_t bufferSuze = sizeof(buffer) / sizeof(char_t);
	if (get_hostfxr_path(buffer, &bufferSuze, nullptr) != 0)
	{
		throw std::runtime_error("Failed to find .NET hostfxr path!");
	}

	m_coreClrLoader = std::make_unique<LibraryLoader>(String(buffer));

	const auto init_res = m_coreClrLoader->TryGetSymbol<hostfxr_initialize_for_runtime_config_fn>("hostfxr_initialize_for_runtime_config");
	const auto delegate_res = m_coreClrLoader->TryGetSymbol<hostfxr_get_runtime_delegate_fn>("hostfxr_get_runtime_delegate");
	const auto close_res = m_coreClrLoader->TryGetSymbol<hostfxr_close_fn>("hostfxr_close");

	if (!init_res || !delegate_res || !close_res)
	{
		throw std::runtime_error("Failed to resolve hostfxr symbols from DLL!");
	}

	const auto init_fptr = *init_res;
	const auto get_delegate_fptr = *delegate_res;
	const auto close_fptr = *close_res;

	hostfxr_handle cxt = nullptr;
	const NativeString config = ToNativeString(runtimeConfig);
	if (init_fptr(config.c_str(), nullptr, &cxt) != 0 || cxt == nullptr)
	{
		throw std::runtime_error("Failed to initialize .NET runtime configuration");
	}

	void* load_assembly_and_get_fptr = nullptr;
	if (const int rc = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_fptr);
		rc != 0 || load_assembly_and_get_fptr == nullptr)
	{
		close_fptr(cxt);
		throw std::runtime_error("Failed to get .NET runtime delegate");
	}

	m_loadAssemblyFn = load_assembly_and_get_fptr;
	close_fptr(cxt);
}

void HostFxrBackend::Shutdown()
{
	m_coreClrLoader.reset();
	m_loadAssemblyFn = nullptr;
}

void* HostFxrBackend::LoadEntryPoint(String const& assemblyPath, String const& typeName, String const& methodName) const
{
	if (!m_loadAssemblyFn)
	{
		throw std::runtime_error("DotNet host is not initialized");
	}

	const auto load_assembly = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(m_loadAssemblyFn);

	const NativeString assembly = ToNativeString(assemblyPath);
	const NativeString type = ToNativeString(typeName);
	const NativeString method = ToNativeString(methodName);

	void* entryPoint = nullptr;
	const int rc = load_assembly(
		assembly.c_str(),
		type.c_str(),
		method.c_str(),
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		&entryPoint);

	if (rc != 0 || entryPoint == nullptr)
	{
		throw std::runtime_error("Failed to load managed entry point!");
	}

	return entryPoint;
}

} // namespace re::scripting
