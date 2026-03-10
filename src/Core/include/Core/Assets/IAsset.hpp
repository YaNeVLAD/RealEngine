#pragma once

#include <Core/String.hpp>

namespace re
{

class IAsset
{
public:
	virtual ~IAsset() = default;

	virtual bool LoadFromFile(String const& filePath) = 0;
};

} // namespace re