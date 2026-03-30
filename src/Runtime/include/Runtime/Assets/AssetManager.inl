#include <Runtime/Assets/AssetManager.hpp>

namespace re
{

template <std::derived_from<IAsset> T>
std::shared_ptr<T> AssetManager::Get(file_system::AssetsPath const& path)
{
	using namespace re::literals;

	const auto str = path.Str();
	if (const auto it = m_assets.find(str); it != m_assets.end())
	{
		return std::dynamic_pointer_cast<T>(it->second);
	}

	auto newAsset = std::make_shared<T>();
	if (newAsset->LoadFromFile(str))
	{
		m_assets[str] = newAsset;
		return newAsset;
	}

	return nullptr;
}

} // namespace re