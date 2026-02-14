#pragma once

#include <Runtime/Export.hpp>

#include <Core/HashedString.hpp>
#include <Core/String.hpp>
#include <RenderCore/Assets/IAsset.hpp>

#include <memory>
#include <unordered_map>

namespace re
{

class RE_RUNTIME_API AssetManager
{
public:
	template <std::derived_from<IAsset> T>
	std::shared_ptr<T> Get(String const& filePath);

	void CleanUp();

private:
	std::unordered_map<Hash_t, std::shared_ptr<IAsset>> m_assets;
};

} // namespace re

#include <Runtime/Assets/AssetManager.inl>