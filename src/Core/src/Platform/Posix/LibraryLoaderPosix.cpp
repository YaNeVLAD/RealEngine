#include <Core/LibraryLoader.hpp>

#include <dlfcn.h>

namespace re
{

namespace
{

String GetDlErrorString()
{
	const char* err = dlerror();
	return err ? String(err) : "Unknown POSIX error";
}

} // namespace

std::expected<LibraryLoader, String> LibraryLoader::Open(String const& path) noexcept
{
	void* handle = dlopen(path.ToString().c_str(), RTLD_NOW);
	if (!handle)
	{
		return std::unexpected("Failed to load library '" + path + "': " + GetDlErrorString());
	}

	return LibraryLoader(handle);
}

void LibraryLoader::Unload() noexcept
{
	if (m_handle)
	{
		dlclose(m_handle);
		m_handle = nullptr;
	}
}

std::expected<void*, String> LibraryLoader::GetSymbolAddress(String const& name) const noexcept
{
	if (!m_handle)
	{
		return std::unexpected("Library is not loaded.");
	}

	dlerror();

	void* symbol = dlsym(m_handle, name.ToString().c_str());
	if (!symbol)
	{
		return std::unexpected("Failed to find symbol '" + name + "': " + GetDlErrorString());
	}

	return symbol;
}

} // namespace re