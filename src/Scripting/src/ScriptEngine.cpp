#include <Scripting/ScriptEngine.hpp>

#include <Core/LibraryLoader.hpp>
#include <Core/String.hpp>
#include <Scripting/Internal/EngineApi.hpp>

#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>

#if defined(RE_SYSTEM_WINDOWS)
#define NET_STR(s) L##s
#else
#define NET_STR(s) s
#endif

#include <iostream>
#include <memory>
#include <stdexcept>

namespace re::scripting
{

void ScriptEngine::InitRVM()
{
	InitNativeLibrary();
	BindEngineAPI(&m_vm);
	// BindPhysicsAPI(m_vm); ...
}

void ScriptEngine::InitCoreCLR()
{
	char_t buffer[1024];
	size_t bufferSuze = sizeof(buffer) / sizeof(char_t);
	if (get_hostfxr_path(buffer, &bufferSuze, nullptr) != 0)
	{
		throw std::runtime_error("Failed to find .NET hostfxr path!");
	}

	String fxrPath(buffer);

	m_coreClrLoader = std::make_unique<LibraryLoader>(fxrPath);

	auto init_fptr = m_coreClrLoader->GetSymbol<hostfxr_initialize_for_runtime_config_fn>("hostfxr_initialize_for_runtime_config");
	auto get_delegate_fptr = m_coreClrLoader->GetSymbol<hostfxr_get_runtime_delegate_fn>("hostfxr_get_runtime_delegate");
	auto close_fptr = m_coreClrLoader->GetSymbol<hostfxr_close_fn>("hostfxr_close");

	const char_t* runtimeConfigPath = NET_STR("EngineAPI.runtimeconfig.json");
	hostfxr_handle cxt = nullptr;

	if (init_fptr(runtimeConfigPath, nullptr, &cxt) != 0 || cxt == nullptr)
	{
		throw std::runtime_error("Failed to initialize .NET runtime configuration!");
	}

	void* load_assembly_and_get_fptr = nullptr;
	int rc = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_fptr);
	if (rc != 0 || load_assembly_and_get_fptr == nullptr)
	{
		close_fptr(cxt);
		throw std::runtime_error("Failed to get .NET runtime delegate!");
	}

	m_loadAssemblyFn = load_assembly_and_get_fptr;
	close_fptr(cxt);

	auto load_assembly = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(m_loadAssemblyFn);

	const char_t* assemblyPath = L"EngineAPI.dll";
	const char_t* typeName = L"EngineAPI.Program, EngineAPI";
	const char_t* methodName = L"InitializeDotNetHost";

	using InitMethod_Fn = void (*)();
	InitMethod_Fn initMethod = nullptr;

	rc = load_assembly(
		assemblyPath,
		typeName,
		methodName,
		UNMANAGEDCALLERSONLY_METHOD,
		nullptr,
		reinterpret_cast<void**>(&initMethod));

	if (rc != 0 || initMethod == nullptr)
	{
		return; // TODO: Add proper build and error handling
		throw std::runtime_error("Failed to load C# method: InitializeDotNetHost from EngineAPI.dll");
	}

	initMethod();
	std::cout << "[ScriptEngine] .NET CoreCLR initialized successfully.\n";
}

} // namespace re::scripting
