#include <Runtime/Main.hpp>

#include "LauncherApplication.hpp"

re::runtime::Application* CreateApplication(int argc, char** argv)
{
	return new LauncherApplication();
}
