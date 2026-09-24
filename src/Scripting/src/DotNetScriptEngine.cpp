#include <Scripting/CSharp/DotNetScriptEngine.hpp>

#include <Core/Logger.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>
#include <Scripting/Host/HostFxrBackend.hpp>
#include <Scripting/Interface/EngineApiPointers.hpp>

#include <stdexcept>

namespace re
{

DotNetScriptEngine::DotNetScriptEngine(const scripting::EngineApiPointers& apiPointers)
{
	InitImpl(apiPointers, String("EngineAPI.runtimeconfig.json"));
}

DotNetScriptEngine::~DotNetScriptEngine()
{
	ShutdownImpl();
}

void DotNetScriptEngine::Init(const scripting::EngineApiPointers& apiPointers)
{
	InitImpl(apiPointers, String("EngineAPI.runtimeconfig.json"));
}

void DotNetScriptEngine::Init(const scripting::EngineApiPointers& apiPointers, String const& runtimeConfig)
{
	InitImpl(apiPointers, runtimeConfig);
}

void DotNetScriptEngine::Shutdown()
{
	ShutdownImpl();
}

bool DotNetScriptEngine::LoadAssembly(String const& filepath)
{
	if (!scripting::DotNetInterop::LoadUserAssembly)
	{
		return false;
	}

	const std::string pathU8 = filepath.ToString();
	const int result = scripting::DotNetInterop::LoadUserAssembly(pathU8.c_str());

	return result != 0;
}

scripting::IScriptClass* DotNetScriptEngine::GetClass(String const& Namespace, String const& Class)
{
	std::string nsU8 = Namespace.ToString();
	std::string clsU8 = Class.ToString();
	const std::string fullName = nsU8.empty() ? clsU8 : nsU8 + "." + clsU8;

	if (const auto it = m_classes.find(fullName); it != m_classes.end())
	{
		return it->second.get();
	}

	if (scripting::DotNetInterop::CheckClassExists && scripting::DotNetInterop::CheckClassExists(fullName.c_str()) != 0)
	{
		auto scriptClass = std::make_unique<DotNetScriptClass>(nsU8, clsU8);

		return (m_classes[fullName] = std::move(scriptClass)).get();
	}

	return nullptr;
}

void* DotNetScriptEngine::LoadManagedEntryPoint(String const& assemblyPath, String const& typeName, String const& methodName) const
{
	if (!m_host)
	{
		throw std::runtime_error("DotNet host is not initialized");
	}

	return m_host->LoadEntryPoint(assemblyPath, typeName, methodName);
}

void DotNetScriptEngine::InitImpl(const scripting::EngineApiPointers& apiPointers, String const& runtimeConfig)
{
	using namespace re::literals;

	if (apiPointers.apiVersion != scripting::EngineApiVersion)
	{
		throw std::runtime_error("Engine API version mismatch! Update C# EngineAPI bindings");
	}

	m_host = std::make_unique<scripting::HostFxrBackend>();
	m_host->Initialize(runtimeConfig);

	RE_LOG_INFO(".NET CoreCLR initialized successfully.");

	auto loadBridgeMethod = [&](const char* methodName, void** outPtr) {
		*outPtr = m_host->LoadEntryPoint(
			"EngineAPI.dll"_s,
			"EngineAPI.Program, EngineAPI"_s,
			String(methodName));
	};

	loadBridgeMethod("LoadUserAssembly", reinterpret_cast<void**>(&scripting::DotNetInterop::LoadUserAssembly));
	loadBridgeMethod("CreateInstance", reinterpret_cast<void**>(&scripting::DotNetInterop::CreateInstance));
	loadBridgeMethod("InvokeOnCreate", reinterpret_cast<void**>(&scripting::DotNetInterop::InvokeOnCreate));
	loadBridgeMethod("InvokeOnUpdate", reinterpret_cast<void**>(&scripting::DotNetInterop::InvokeOnUpdate));
	loadBridgeMethod("FreeInstance", reinterpret_cast<void**>(&scripting::DotNetInterop::FreeInstance));
	loadBridgeMethod("OnDestroy", reinterpret_cast<void**>(&scripting::DotNetInterop::OnDestroy));
	loadBridgeMethod("CheckClassExists", reinterpret_cast<void**>(&scripting::DotNetInterop::CheckClassExists));
	loadBridgeMethod("InvokeMethodByName", reinterpret_cast<void**>(&scripting::DotNetInterop::InvokeMethodByName));

	RE_LOG_INFO("CSharp Bridge delegates loaded successfully.");
}

void DotNetScriptEngine::ShutdownImpl()
{
	m_classes.clear();

	if (m_host)
	{
		m_host->Shutdown();
	}
	m_host.reset();
}

} // namespace re
