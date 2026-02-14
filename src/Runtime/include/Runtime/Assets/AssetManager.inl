#include <Runtime/Assets/AssetManager.hpp>

namespace re
{

template <std::derived_from<IAsset> T>
std::shared_ptr<T> AssetManager::Get(String const& filePath)
{
	using namespace re::literals;

	const auto hash = HashedU32String::Value(filePath.Data(), filePath.Length());
	if (const auto it = m_assets.find(hash); it != m_assets.end())
	{
		return std::dynamic_pointer_cast<T>(it->second);
	}

	auto newAsset = std::make_shared<T>();
	if (newAsset->LoadFromFile(filePath))
	{
		m_assets[hash] = newAsset;
		return newAsset;
	}

	return nullptr;
}

} // namespace re