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
	explicit LibraryLoader(String const& path);

	~LibraryLoader();

	template <typename T>
	T GetSymbol(String const& name) const;

private:
	HMODULE m_handle = nullptr;
};

} // namespace re

#include <Core/LibraryLoader.inl>

#endif
