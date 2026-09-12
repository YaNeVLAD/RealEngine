#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API Font : public IAsset
{
public:
	Font() = default;

	explicit Font(const std::string& filepath);
	Font(const void* data, std::size_t size);

	bool LoadFromFile(const std::string& filepath);
	bool LoadFromMemory(const void* data, std::size_t size);

	[[nodiscard]] const std::vector<std::uint8_t>& GetFontData() const { return m_fontData; }
	[[nodiscard]] bool IsLoaded() const { return !m_fontData.empty(); }

	bool LoadFromFile(String const& filePath, const AssetManager* manager) override
	{
		return true;
	}

	// TODO: Сюда нужно перенести геттеры специфичные для вашего старого API,
	// например, GetTexture() или GetGlyph(char32_t charCode), если вы парсите метрики прямо здесь.

private:
	std::vector<std::uint8_t> m_fontData;
};

} // namespace re