#include <Core/FileSystem.hpp>

#include <vector>

#include <unistd.h>

namespace re::file_system::raw
{

std::filesystem::path GetExecutablePath()
{
	std::vector<char> buffer(1024);
	ssize_t len = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);

	if (len != -1)
	{
		buffer[len] = '\0';
		return std::filesystem::path(buffer.data());
	}

	return {};
}

} // namespace re::file_system::raw