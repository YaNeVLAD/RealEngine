#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

#include <cstdint>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API Font : public IAsset
{
public:
	Font() = default;

	explicit Font(const String& filepath);
	Font(const void* data, std::size_t size);

	bool LoadFromMemory(const void* data, std::size_t size);

	[[nodiscard]] const std::vector<std::uint8_t>& GetFontData() const;
	[[nodiscard]] bool IsLoaded() const;

	bool LoadFromFile(String const& filePath, const AssetManager* manager) override;

private:
	std::vector<std::uint8_t> m_fontData;
};

} // namespace re