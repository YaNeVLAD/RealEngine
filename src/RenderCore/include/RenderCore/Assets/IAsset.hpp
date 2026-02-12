#pragma once

#include <string>

namespace re
{

class IAsset
{
public:
	virtual ~IAsset() = default;

	virtual bool LoadFromFile(std::string const& filePath) = 0;
};

} // namespace re