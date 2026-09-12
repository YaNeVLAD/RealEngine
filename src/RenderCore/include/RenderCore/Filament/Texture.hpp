#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

#include <cstdint>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API Texture : public IAsset
{
public:
	Texture() = default;

	Texture(const void* data, std::uint32_t width, std::uint32_t height, std::uint32_t channels);
	Texture(std::uint32_t width, std::uint32_t height);

	[[nodiscard]] std::uint32_t Width() const;
	[[nodiscard]] std::uint32_t Height() const;
	[[nodiscard]] std::uint32_t Channels() const;
	[[nodiscard]] bool IsSRGB() const;
	[[nodiscard]] String const& GetFilePath() const;

	[[nodiscard]] const std::vector<std::uint8_t>& GetPixelData() const;

	void SetData(const void* data, std::size_t size);

	bool LoadFromFile(String const& filePath, const AssetManager* manager) override;

	bool LoadFromFileSRGB(String const& filePath, bool srgb);

	bool LoadFromMemorySRGB(const std::uint8_t* data, std::size_t size, bool srgb = false);

private:
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	std::uint32_t m_channels{};

	std::vector<std::uint8_t> m_pixelData{};

	bool m_isSRGB = false;
	String m_filePath{};
};

} // namespace re