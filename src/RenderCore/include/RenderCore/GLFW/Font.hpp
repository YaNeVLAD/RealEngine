#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

namespace re
{

class RE_RENDER_CORE_API Font final : public IAsset
{
public:
	bool LoadFromFile(String const& filePath, const AssetManager* manager) override;

	[[nodiscard]] void* GetNativeHandle() const;
};

} // namespace re