#include <Runtime/Main.hpp>

#include "LauncherApplication.hpp"
#include "ScriptCompiler.hpp"

re::Application* CreateApplication(int argc, char** argv)
{
	launcher::CompileScripts();

	return new LauncherApplication();
}
