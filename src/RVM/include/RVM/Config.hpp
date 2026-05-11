#pragma once

#include <cstddef>

namespace re::rvm
{

struct Config
{
	bool autoProcessDestructors;
	bool autoCollectCycles;
	std::size_t gcThreshold;

	static Config Default()
	{
		return Enabled();
	}

	static Config Enabled()
	{
		return Config{ true, true, 1024 };
	}

	static Config Disabled()
	{
		return Config{ false, false, 1024 };
	}
};

} // namespace re::rvm