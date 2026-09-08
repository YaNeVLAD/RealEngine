#pragma once

#include <Core/Export.hpp>

#include <Core/String.hpp>

#include <filesystem>

namespace re::file_system
{

namespace raw
{

inline constexpr char ASSETS_DIR[] = "assets";
inline constexpr char SHADERS_DIR[] = "shaders";
inline constexpr char SCRIPTS_DIR[] = "scripts";
inline constexpr char BIN_DIR[] = "bin";

[[nodiscard]] RE_CORE_API std::filesystem::path GetExecutablePath();
[[nodiscard]] RE_CORE_API std::filesystem::path GetBasePath();

template <const char* PARENT_DIR>
class ResourcePath
{
public:
	ResourcePath(const char* path);

	ResourcePath(String const& path);

	ResourcePath(std::nullptr_t) = delete;

	[[nodiscard]] String Str() const;

private:
	static std::filesystem::path MakeAbsolute(std::filesystem::path const& path);

private:
	std::filesystem::path m_path;
};

} // namespace raw

using AssetsPath = raw::ResourcePath<raw::ASSETS_DIR>;
using BinaryPath = raw::ResourcePath<raw::BIN_DIR>;
using ScriptsPath = raw::ResourcePath<raw::SCRIPTS_DIR>;
using ShadersPath = raw::ResourcePath<raw::SHADERS_DIR>;

namespace literals
{

inline AssetsPath operator""_asset(const char* str, std::size_t /*len*/)
{
	return AssetsPath(String(str));
}

inline BinaryPath operator""_binary(const char* str, std::size_t /*len*/)
{
	return BinaryPath(String(str));
}

inline ScriptsPath operator""_script(const char* str, std::size_t /*len*/)
{
	return ScriptsPath(String(str));
}

inline ShadersPath operator""_shader(const char* str, std::size_t /*len*/)
{
	return ShadersPath(String(str));
}

} // namespace literals

} // namespace re::file_system

#include <Core/FileSystem.inl>