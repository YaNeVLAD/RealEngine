#pragma once

#include <Runtime/Application.hpp>

extern re::Application* CreateApplication(int argc, char** argv);

namespace re::runtime
{

inline int Main(const int argc, char** argv)
{
	const auto app = CreateApplication(argc, argv);
	app->Run();

	delete app;

	return 0;
}

} // namespace re::runtime

#if defined(RE_SYSTEM_WINDOWS) && defined(RE_DIST)
#define NOMINMAX
#include <Windows.h>

inline int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, PSTR cmdline, int show)
{
	return re::runtime::Main(__argc, __argv);
}

#else

inline int main(const int argc, char** argv)
{
	return re::runtime::Main(argc, argv);
}

#endif