#pragma once

#include <RenderCore/Export.hpp>

#include <Core/String.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace re::render
{

enum class TextureFormat : std::uint8_t
{
	RGB,
	RGBA,
	DXT1,
	DXT3,
	DXT5,
};

struct TextureData
{
	std::uint32_t width{};
	std::uint32_t height{};

	std::uint32_t mipMapCount = 1;

	TextureFormat format = TextureFormat::RGBA;
	std::vector<std::uint8_t> buffer;
};

RE_RENDER_CORE_API std::optional<TextureData> LoadTexture(String const& filePath);

} // namespace re::render