#include <Runtime/Main.hpp>

#include <Scripting/ScriptCompiler.hpp>

#include "LauncherApplication.hpp"

re::Application* CreateApplication(int /*argc*/, char** /*argv*/)
{
	re::scripting::ScriptCompiler::CompileAllModified();

	return new LauncherApplication();
}
