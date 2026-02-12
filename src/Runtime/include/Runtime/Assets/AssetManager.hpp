#pragma once

#include <Runtime/Export.hpp>

#include <Core/Meta/TypeInfo.hpp>
#include <RenderCore/Assets/IAsset.hpp>

#include <memory>
#include <unordered_map>

namespace re
{

class RE_RUNTIME_API AssetManager
{
public:
	template <std::derived_from<IAsset> T>
	std::shared_ptr<T> Get(std::string const& filePath);

	void CleanUp();

private:
	std::unordered_map<meta::TypeHashType, std::shared_ptr<IAsset>> s_assets;
};

} // namespace re

#include <Runtime/Assets/AssetManager.inl>