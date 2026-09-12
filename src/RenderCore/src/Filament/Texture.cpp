#include <RenderCore/Filament/Texture.hpp>

#include <RenderCore/LoadTexture.hpp>

#include <Core/Math/Color.hpp>

#include <bcdec.h>
#include <stb_image.h>

#include <array>
#include <cstring>

namespace
{

std::vector<std::uint8_t> DecodeDxt(const std::vector<std::uint8_t>& data, const std::uint32_t width, const std::uint32_t height, const re::render::TextureFormat format)
{
	const std::uint32_t blockWidth = (width + 3) / 4;
	const std::uint32_t blockHeight = (height + 3) / 4;
	const std::size_t blockSize = (format == re::render::TextureFormat::DXT1) ? 8 : 16;

	const std::size_t needed = static_cast<std::size_t>(blockWidth) * blockHeight * blockSize;
	if (data.size() < needed)
	{
		return {};
	}

	std::vector<std::uint8_t> result(static_cast<std::size_t>(width) * height * 4, 0);

	std::size_t offset = 0;
	for (std::uint32_t by = 0; by < blockHeight; ++by)
	{
		for (std::uint32_t bx = 0; bx < blockWidth; ++bx)
		{
			const std::uint8_t* block = data.data() + offset;
			offset += blockSize;

			std::array<std::uint8_t, 64> blockPixels{};
			switch (format)
			{
			case re::render::TextureFormat::DXT1:
				bcdec_bc1(block, blockPixels.data(), 16);
				break;
			case re::render::TextureFormat::DXT3:
				bcdec_bc2(block, blockPixels.data(), 16);
				break;
			case re::render::TextureFormat::DXT5:
				bcdec_bc3(block, blockPixels.data(), 16);
				break;
			default:
				return {};
			}

			for (std::uint32_t py = 0; py < 4; ++py)
			{
				const std::uint32_t y = by * 4 + py;
				if (y >= height)
				{
					continue;
				}

				for (std::uint32_t px = 0; px < 4; ++px)
				{
					const std::uint32_t x = bx * 4 + px;
					if (x >= width)
					{
						continue;
					}

					const std::uint8_t* src = blockPixels.data() + (py * 4 + px) * 4;
					std::uint8_t* dst = &result[(static_cast<std::size_t>(y) * width + x) * 4];
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
				}
			}
		}
	}

	return result;
}

} // namespace

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
	else [[unlikely]]
	{
		m_width = 1;
		m_height = 1;
		m_channels = 4;
		m_pixelData = { 255, 0, 255, 255 };
	}
}

Texture::Texture(const std::uint32_t width, const std::uint32_t height)
	: m_width(width)
	, m_height(height)
	, m_channels(sizeof(Color))
{
}

std::uint32_t Texture::Width() const
{
	return m_width;
}

std::uint32_t Texture::Height() const
{
	return m_height;
}

std::uint32_t Texture::Channels() const
{
	return m_channels;
}

bool Texture::IsSRGB() const
{
	return m_isSRGB;
}

String const& Texture::GetFilePath() const
{
	return m_filePath;
}

const std::vector<std::uint8_t>& Texture::GetPixelData() const
{
	return m_pixelData;
}

void Texture::SetData(const void* data, const std::size_t size)
{
	if (data && size > 0)
	{
		m_pixelData.resize(size);
		std::memcpy(m_pixelData.data(), data, size);
	}
}

bool Texture::LoadFromFile(String const& filePath, const AssetManager*)
{
	return LoadFromFileSRGB(filePath, false);
}

bool Texture::LoadFromFileSRGB(String const& filePath, const bool srgb)
{
	const auto data = render::LoadTexture(filePath);
	if (!data)
	{
		return false;
	}

	m_filePath = filePath;
	m_isSRGB = srgb;
	m_width = data->width;
	m_height = data->height;
	m_channels = 0;
	m_pixelData.clear();

	switch (data->format)
	{
	case render::TextureFormat::RGB:
		m_channels = 3;
		m_pixelData = data->buffer;
		break;

	case render::TextureFormat::RGBA:
		m_channels = 4;
		m_pixelData = data->buffer;
		break;

	case render::TextureFormat::DXT1:
	case render::TextureFormat::DXT3:
	case render::TextureFormat::DXT5:
		m_channels = 4;
		m_pixelData = DecodeDxt(data->buffer, data->width, data->height, data->format);
		if (m_pixelData.empty())
		{
			m_width = 0;
			m_height = 0;

			return false;
		}
		break;
	}

	return m_channels != 0;
}

bool Texture::LoadFromMemorySRGB(const std::uint8_t* data, std::size_t size, const bool srgb)
{
	int width = 0;
	int height = 0;
	int channelsInFile = 0;

	stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channelsInFile, STBI_rgb_alpha);
	if (!pixels)
	{
		return false;
	}

	m_width = static_cast<std::uint32_t>(width);
	m_height = static_cast<std::uint32_t>(height);
	m_channels = STBI_rgb_alpha;
	m_isSRGB = srgb;
	m_filePath.Clear();

	const std::size_t pixelSize = static_cast<std::size_t>(width) * height * 4;
	m_pixelData.assign(pixels, pixels + pixelSize);

	stbi_image_free(pixels);

	return true;
}

} // namespace re