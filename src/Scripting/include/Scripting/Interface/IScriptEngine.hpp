#pragma once

#include <Core/FileSystem.hpp>
#include <Core/String.hpp>
#include <Scripting/Interface/IScriptClass.hpp>

namespace re::scripting
{

class IScriptEngine
{
public:
	virtual ~IScriptEngine() = default;

	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	virtual bool LoadAssembly(String const& filepath) = 0;

	bool LoadAssembly(file_system::ScriptsPath const& filepath)
	{
		return LoadAssembly(filepath.Str());
	}

	virtual IScriptClass* GetClass(String const& Namespace, String const& Class) = 0;
};

} // namespace re::scripting
