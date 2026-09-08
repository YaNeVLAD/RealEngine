#include <Core/LibraryLoader.hpp>

namespace re
{

LibraryLoader::LibraryLoader(String const& path)
{
	auto expected = Open(path);
	if (!expected)
	{
		throw std::runtime_error{ "Cannot open library: " + path };
	}

	m_handle = std::exchange(expected->m_handle, nullptr);
}

LibraryLoader::LibraryLoader(LibraryLoader&& other) noexcept
	: m_handle(std::exchange(other.m_handle, nullptr))
{
}

LibraryLoader& LibraryLoader::operator=(LibraryLoader&& other) noexcept
{
	if (this != &other)
	{
		Unload();
		m_handle = std::exchange(other.m_handle, nullptr);
	}

	return *this;
}

LibraryLoader::~LibraryLoader()
{
	Unload();
}

bool LibraryLoader::IsLoaded() const noexcept
{
	return m_handle != nullptr;
}

LibraryLoader::LibraryLoader(void* handle) noexcept
	: m_handle(handle)
{
}

} // namespace re
