#include <Runtime/Assets/AssetManager.hpp>

namespace re
{

void AssetManager::CleanUp()
{
	for (auto it = s_assets.begin(); it != s_assets.end();)
	{
		if (it->second.use_count() == 1)
		{
			it = s_assets.erase(it);
		}
		else
		{
			++it;
		}
	}
}

} // namespace re