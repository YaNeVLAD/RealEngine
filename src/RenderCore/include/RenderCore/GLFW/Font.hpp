#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Assets/IAsset.hpp>

#include <string>

namespace re
{

class RE_RENDER_CORE_API Font final : public IAsset
{
public:
	bool LoadFromFile(String const& filePath) override;

	[[nodiscard]] void* GetNativeHandle() const;
};

} // namespace re