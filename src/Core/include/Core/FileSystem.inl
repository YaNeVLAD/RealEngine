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
	: m_path(MakeAbsolute(std::filesystem::path(PARENT_DIR) / path.Data()))
{
}

template <const char* PARENT_DIR>
String ResourcePath<PARENT_DIR>::Str() const
{
	return m_path.string();
}

template <const char* PARENT_DIR>
std::filesystem::path ResourcePath<PARENT_DIR>::FetchExecutablePath()
{
#ifdef RE_SYSTEM_WINDOWS
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(nullptr, buffer, MAX_PATH);

	return { buffer };
#endif
}

template <const char* PARENT_DIR>
std::filesystem::path ResourcePath<PARENT_DIR>::GetBasePath()
{
	static std::filesystem::path path = FetchExecutablePath().parent_path();

	return path;
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