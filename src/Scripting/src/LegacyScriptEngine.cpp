#include <Scripting/LegacyScriptEngine.hpp>

#include <Core/LibraryLoader.hpp>
#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
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

void LegacyScriptEngine::InitRVM()
{
	InitNativeLibrary();
	BindEngineAPI(&m_vm);
	// BindPhysicsAPI(m_vm); ...
}

void LegacyScriptEngine::InitCoreCLR()
{
	char_t buffer[1024];
	size_t bufferSuze = sizeof(buffer) / sizeof(char_t);
	if (get_hostfxr_path(buffer, &bufferSuze, nullptr) != 0)
	{
		throw std::runtime_error("Failed to find .NET hostfxr path!");
	}

	String fxrPath(buffer);

	m_coreClrLoader = std::make_unique<LibraryLoader>(fxrPath);

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
	if (init_fptr(NET_STR("EngineAPI.runtimeconfig.json"), nullptr, &cxt) != 0 || cxt == nullptr)
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

	const auto load_assembly = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(m_loadAssemblyFn);

	const auto assemblyPath = L"EngineAPI.dll";
	const auto typeName = L"EngineAPI.Program, EngineAPI";
	const auto methodName = L"InitializeDotNetHost";

	using InitMethod_Fn = void (*)(EngineApiPointers*);
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
		throw std::runtime_error("Failed to load C# method: InitializeDotNetHost from EngineAPI.dll");
	}

	EngineApiPointers apiPointers{};
	apiPointers.NativeLog = &internal::NativeLog_Impl;

	initMethod(&apiPointers);
	std::cout << "[ScriptEngine] .NET CoreCLR initialized successfully.\n";

	auto load_csharp_method = [&](const char_t* method_name, void** out_ptr) {
		const int res = load_assembly(
			assemblyPath,
			typeName,
			method_name,
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			out_ptr);

		if (res != 0 || *out_ptr == nullptr)
		{
			throw std::runtime_error("Failed to load C# bridge method!");
		}
	};

	load_csharp_method(NET_STR("LoadUserAssembly"), reinterpret_cast<void**>(&DotNetInterop::LoadUserAssembly));
	load_csharp_method(NET_STR("CreateInstance"), reinterpret_cast<void**>(&DotNetInterop::CreateInstance));
	load_csharp_method(NET_STR("InvokeOnCreate"), reinterpret_cast<void**>(&DotNetInterop::InvokeOnCreate));
	load_csharp_method(NET_STR("InvokeOnUpdate"), reinterpret_cast<void**>(&DotNetInterop::InvokeOnUpdate));
	load_csharp_method(NET_STR("FreeInstance"), reinterpret_cast<void**>(&DotNetInterop::FreeInstance));

	std::cout << "[ScriptEngine] C# Bridge delegates loaded successfully.\n";
}

} // namespace re::scripting
