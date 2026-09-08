#include <Core/LibraryLoader.hpp>

#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace re
{

namespace
{
String GetLastWin32Error()
{
	const DWORD errorMessageId = ::GetLastError();
	if (errorMessageId == 0)
	{
		return "Unknown Win32 error";
	}

	LPWSTR messageBuffer = nullptr;
	const std::size_t size = FormatMessageW(
		   FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		   nullptr, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		   reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

	std::wstring message(messageBuffer, size);
	LocalFree(messageBuffer);

	return message;
}
} // namespace

std::expected<LibraryLoader, String> LibraryLoader::Open(String const& path) noexcept
{
	const HMODULE handle = LoadLibraryW(path.ToWString().c_str());
	if (!handle)
	{
		return std::unexpected("Failed to load library '" + path + "': " + GetLastWin32Error());
	}

	return LibraryLoader(static_cast<void*>(handle));
}

void LibraryLoader::Unload() noexcept
{
	if (m_handle)
	{
		FreeLibrary(static_cast<HMODULE>(m_handle));
		m_handle = nullptr;
	}
}

std::expected<void*, String> LibraryLoader::GetSymbolAddress(String const& name) const noexcept
{
	if (!m_handle)
	{
		return std::unexpected("Library is not loaded.");
	}

	const auto handle = static_cast<HMODULE>(m_handle);
	auto symbol = reinterpret_cast<void*>(GetProcAddress(handle, name.ToString().c_str()));
	if (!symbol)
	{
		return std::unexpected("Failed to find symbol '" + name + "': " + GetLastWin32Error());
	}

	return symbol;
}

} // namespace re