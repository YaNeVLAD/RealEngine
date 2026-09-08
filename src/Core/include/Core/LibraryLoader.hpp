#pragma once

#include <Core/Export.hpp>

#include <Core/String.hpp>

#include <expected>
#include <stdexcept>

namespace re
{

class RE_CORE_API LibraryLoader
{
public:
	LibraryLoader() noexcept = default;
	explicit LibraryLoader(String const& path);

	LibraryLoader(LibraryLoader const&) = delete;
	LibraryLoader& operator=(LibraryLoader const&) = delete;

	LibraryLoader(LibraryLoader&& other) noexcept;
	LibraryLoader& operator=(LibraryLoader&& other) noexcept;

	~LibraryLoader();

	[[nodiscard]] static std::expected<LibraryLoader, String> Open(String const& path) noexcept;

	template <typename T>
	T GetSymbol(String const& name) const;

	template <typename T>
	[[nodiscard]] std::expected<T, String> TryGetSymbol(String const& name) const noexcept;

	[[nodiscard]] bool IsLoaded() const noexcept;

private:
	explicit LibraryLoader(void* handle) noexcept;

	[[nodiscard]] std::expected<void*, String> GetSymbolAddress(String const& name) const noexcept;
	void Unload() noexcept;

private:
	void* m_handle = nullptr;
};

} // namespace re

#include <Core/LibraryLoader.inl>