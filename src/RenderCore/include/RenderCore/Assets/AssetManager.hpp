#pragma once

#include <RenderCore/Export.hpp>

#include <Core/FileSystem.hpp>
#include <Core/String.hpp>
#include <RenderCore/Assets/IAsset.hpp>

#include <memory>
#include <unordered_map>

namespace re
{

class RE_RENDER_CORE_API AssetManager
{
public:
	template <std::derived_from<IAsset> T>
	std::shared_ptr<T> Get(file_system::AssetsPath const& path) const;

	template <std::derived_from<IAsset> T>
	void Add(file_system::AssetsPath const& name, std::shared_ptr<T> asset);

	void CleanUp();

private:
	mutable std::unordered_map<String, std::shared_ptr<IAsset>> m_assets;
};

} // namespace re

#include <RenderCore/Assets/AssetManager.inl>