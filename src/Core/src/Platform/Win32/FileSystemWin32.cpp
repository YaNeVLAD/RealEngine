#include <Core/FileSystem.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace re::file_system::raw
{

std::filesystem::path GetExecutablePath()
{
	wchar_t buffer[MAX_PATH];
	if (const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		length == 0 || length == MAX_PATH)
	{
		return {};
	}

	return std::filesystem::path(buffer);
}

} // namespace re::file_system::raw