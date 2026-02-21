#pragma once

#include <Core/Export.hpp>

#include <Core/String.hpp>

#include <filesystem>

#ifdef RE_SYSTEM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#endif

namespace re::file_system
{

namespace raw
{

inline constexpr char ASSETS_DIR[] = "assets";
inline constexpr char BIN_DIR[] = "bin";

template <const char* PARENT_DIR>
class ResourcePath
{
public:
	ResourcePath(const char* path);

	ResourcePath(String const& path);

	ResourcePath(std::nullptr_t) = delete;

	[[nodiscard]] String Str() const;

private:
	static std::filesystem::path FetchExecutablePath();

	static std::filesystem::path GetBasePath();

	static std::filesystem::path MakeAbsolute(std::filesystem::path const& path);

private:
	std::filesystem::path m_path;
};

} // namespace raw

using AssetsPath = raw::ResourcePath<raw::ASSETS_DIR>;
using BinaryPath = raw::ResourcePath<raw::BIN_DIR>;

} // namespace re::file_system

#include <Core/FileSystem.inl>