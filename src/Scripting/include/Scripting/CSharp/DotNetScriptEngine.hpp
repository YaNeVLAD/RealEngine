#pragma once

#include <Scripting/Export.hpp>

#include <Core/LibraryLoader.hpp>
#include <Core/String.hpp>
#include <Scripting/CSharp/DotNetScriptClass.hpp>
#include <Scripting/Interface/IScriptEngine.hpp>

#include <memory>
#include <unordered_map>

namespace re
{

class RE_SCRIPTING_API DotNetScriptEngine final : public scripting::IScriptEngine
{
public:
	using IScriptEngine::LoadAssembly;

	DotNetScriptEngine() = default;
	explicit DotNetScriptEngine(const scripting::EngineApiPointers& apiPointers);

	DotNetScriptEngine(const DotNetScriptEngine&) = delete;
	DotNetScriptEngine& operator=(const DotNetScriptEngine&) = delete;

	DotNetScriptEngine(DotNetScriptEngine&&) noexcept = default;
	DotNetScriptEngine& operator=(DotNetScriptEngine&&) noexcept = default;

	~DotNetScriptEngine() override;

	void Init(const scripting::EngineApiPointers& apiPointers) override;

	void Shutdown() override;

	bool LoadAssembly(String const& filepath) override;

	scripting::IScriptClass* GetClass(String const& Namespace, String const& Class) override;

private:
	void InitImpl(const scripting::EngineApiPointers& apiPointers);
	void ShutdownImpl();

private:
	std::unordered_map<String, std::unique_ptr<DotNetScriptClass>> m_classes;

	std::unique_ptr<LibraryLoader> m_coreClrLoader;
	void* m_loadAssemblyFn = nullptr;
};

} // namespace re
