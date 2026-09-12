#include <RenderCore/Filament/Texture.hpp>

#include <Core/Math/Color.hpp>

#include <cstring>
#include <stdexcept>

namespace re
{

Texture::Texture(const void* data, const std::uint32_t width, const std::uint32_t height, const std::uint32_t channels)
	: m_width(width)
	, m_height(height)
	, m_channels(channels)
{
	if (data && width > 0 && height > 0 && channels > 0)
	{
		const std::size_t dataSize = static_cast<std::size_t>(width) * height * channels;
		m_pixelData.resize(dataSize);
		std::memcpy(m_pixelData.data(), data, dataSize);
	}
	else
	{
		// Можно залогировать предупреждение или создать 1x1 текстуру-заглушку (magenta/checkerboard)
		m_width = 1;
		m_height = 1;
		m_channels = 4;
		m_pixelData = { 255, 0, 255, 255 }; // Маджента (ошибка загрузки)
	}
}

Texture::Texture(const std::uint32_t width, const std::uint32_t height)
	: m_width(width)
	, m_height(height)
	, m_channels(sizeof(Color))
{
}

void Texture::SetData(const void* data, const std::size_t size)
{
	if (data && size > 0)
	{
		m_pixelData.resize(size);
		std::memcpy(m_pixelData.data(), data, size);
	}
}

bool Texture::LoadFromFile(String const& filePath, const AssetManager* manager)
{
	return true;
}

bool Texture::LoadFromFileSRGB(String const& filePath, bool srgb)
{
	return true;
}

bool Texture::LoadFromMemorySRGB(const std::uint8_t* data, std::size_t size, bool srgb)
{
	return true;
}

} // namespace re