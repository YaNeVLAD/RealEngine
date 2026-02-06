#include <Runtime/Main.hpp>

#include "LauncherApplication.hpp"

re::Application* CreateApplication(int argc, char** argv)
{
	return new LauncherApplication();
}
