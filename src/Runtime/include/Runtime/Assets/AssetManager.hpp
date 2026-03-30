#pragma once

#include <Runtime/Export.hpp>

#include <Core/Assets/IAsset.hpp>
#include <Core/FileSystem.hpp>
#include <Core/String.hpp>

#include <memory>
#include <unordered_map>

namespace re
{

class RE_RUNTIME_API AssetManager
{
public:
	template <std::derived_from<IAsset> T>
	std::shared_ptr<T> Get(file_system::AssetsPath const& path);

	void CleanUp();

private:
	std::unordered_map<String, std::shared_ptr<IAsset>> m_assets;
};

} // namespace re

#include <Runtime/Assets/AssetManager.inl>