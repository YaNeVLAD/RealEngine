#pragma once

#include <Core/Export.hpp>

#include <Core/String.hpp>

#if defined(RE_SYSTEM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace re
{

class LibraryLoader
{
public:
	explicit LibraryLoader(String const& path)
	{
		if (m_handle = LoadLibraryA(path.ToString().c_str()); !m_handle)
		{
			throw std::runtime_error("Failed to load library: " + path);
		}
	}

	~LibraryLoader()
	{
		if (m_handle)
		{
			FreeLibrary(static_cast<HMODULE>(m_handle));
		}
	}

	template <typename T>
	T GetSymbol(String const& name) const
	{
		auto symbol = (void*)GetProcAddress(static_cast<HMODULE>(m_handle), name.ToString().c_str());
		if (!symbol)
		{
			throw std::runtime_error("Failed to find symbol: " + name);
		}

		return reinterpret_cast<T>(symbol);
	}

private:
	void* m_handle = nullptr;
};

} // namespace re

#endif
