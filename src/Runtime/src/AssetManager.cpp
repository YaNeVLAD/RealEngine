#include <Runtime/Assets/AssetManager.hpp>

namespace re
{

void AssetManager::CleanUp()
{
	for (auto it = m_assets.begin(); it != m_assets.end();)
	{
		if (it->second.use_count() == 1)
		{
			it = m_assets.erase(it);
		}
		else
		{
			++it;
		}
	}
}

} // namespace re