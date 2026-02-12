#include <Runtime/Assets/AssetManager.hpp>

namespace re
{

template <std::derived_from<IAsset> T>
std::shared_ptr<T> AssetManager::Get(std::string const& filePath)
{
	const auto hash = meta::HashStr(filePath);
	if (const auto it = s_assets.find(hash); it != s_assets.end())
	{
		return std::dynamic_pointer_cast<T>(it->second);
	}

	auto newAsset = std::make_shared<T>();
	if (newAsset->LoadFromFile(filePath))
	{
		s_assets[hash] = newAsset;
		return newAsset;
	}

	return nullptr;
}

} // namespace re