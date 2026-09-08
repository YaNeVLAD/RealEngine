#include <Core/FileSystem.hpp>

#include <vector>

#include <mach-o/dyld.h>
#include <unistd.h>

namespace re::file_system::raw
{

std::filesystem::path GetExecutablePath()
{
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	if (size > 0)
	{
		std::vector<char> buffer(size);
		if (_NSGetExecutablePath(buffer.data(), &size) == 0)
		{
			return std::filesystem::canonical(buffer.data());
		}
	}

	return {};
}

} // namespace re::file_system::raw