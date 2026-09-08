#include <Core/FileSystem.hpp>

namespace re::file_system::raw
{

std::filesystem::path GetBasePath()
{
	static const std::filesystem::path basePath = GetExecutablePath().parent_path();

	return basePath;
}

} // namespace re::file_system::raw