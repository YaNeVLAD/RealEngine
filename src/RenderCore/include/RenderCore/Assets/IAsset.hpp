#pragma once

#include <Core/String.hpp>

namespace re
{

class AssetManager;

class IAsset
{
public:
	virtual ~IAsset() = default;

	virtual bool LoadFromFile(String const& filePath, const AssetManager* manager) = 0;
};

} // namespace re