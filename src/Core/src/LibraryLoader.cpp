#include <Core/LibraryLoader.hpp>

namespace re
{

LibraryLoader::LibraryLoader(String const& path)
{
	if (m_handle = LoadLibraryA(path.ToString().c_str()); !m_handle)
	{
		throw std::runtime_error("Failed to load library: " + path);
	}
}

LibraryLoader::~LibraryLoader()
{
	if (m_handle)
	{
		FreeLibrary(m_handle);
	}
}

} // namespace re