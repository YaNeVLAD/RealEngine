#include <Core/FileSystem.hpp>

namespace re::file_system::raw
{

template <const char* PARENT_DIR>
ResourcePath<PARENT_DIR>::ResourcePath(const char* path)
	: m_path(MakeAbsolute(std::filesystem::path(PARENT_DIR) / path))
{
}

template <const char* PARENT_DIR>
ResourcePath<PARENT_DIR>::ResourcePath(String const& path)
	: m_path(MakeAbsolute(std::filesystem::path(PARENT_DIR) / path.ToWString()))
{
}

template <const char* PARENT_DIR>
String ResourcePath<PARENT_DIR>::Str() const
{
	return String(m_path.native().c_str());
}

template <const char* PARENT_DIR>
std::filesystem::path ResourcePath<PARENT_DIR>::MakeAbsolute(std::filesystem::path const& path)
{
	if (path.is_absolute())
	{
		return path;
	}
	const auto relative = path.has_root_directory() ? path.relative_path() : path;

	return GetBasePath() / relative;
}

} // namespace re::file_system::raw