#pragma once

#include <Runtime/Application.hpp>

#if defined(RE_SYSTEM_WINDOWS)

#define NOMINMAX
#include <windows.h>

extern "C" {
// ReSharper disable once CppNonInlineVariableDefinitionInHeaderFile

// Points NVIDIA to use discrete GPU
__declspec(dllexport) DWORD NvOptimusEnablement = 1;

// ReSharper disable once CppNonInlineVariableDefinitionInHeaderFile

// Points AMD to use discrete GPU
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

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