#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

#include <string>

namespace re
{

class RE_RENDER_CORE_API Font final : public IAsset
{
public:
	bool LoadFromFile(std::string const& filePath) override;

	[[nodiscard]] void* GetNativeHandle() const;
};

} // namespace re