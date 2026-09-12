#include <RenderCore/Filament/Texture.hpp>

#include <RenderCore/LoadTexture.hpp>

#include <Core/Math/Color.hpp>

#include <stb_image.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace
{

struct RgbaPixel
{
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;
	std::uint8_t a = 255;
};

std::uint16_t LoadLE16(const std::uint8_t* bytes)
{
	return static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1]) << 8);
}

RgbaPixel Expand565(const std::uint16_t value)
{
	const std::uint8_t r5 = static_cast<std::uint8_t>((value >> 11) & 0x1F);
	const std::uint8_t g6 = static_cast<std::uint8_t>((value >> 5) & 0x3F);
	const std::uint8_t b5 = static_cast<std::uint8_t>(value & 0x1F);

	return RgbaPixel{
		static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2)),
		static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4)),
		static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2)),
		255,
	};
}

void DecodeColorBlock(const std::uint8_t* block, RgbaPixel out[16])
{
	const std::uint16_t c0 = LoadLE16(block);
	const std::uint16_t c1 = LoadLE16(block + 2);

	RgbaPixel palette[4];
	palette[0] = Expand565(c0);
	palette[1] = Expand565(c1);

	if (c0 > c1)
	{
		palette[2] = {
			static_cast<std::uint8_t>((2 * palette[0].r + palette[1].r) / 3),
			static_cast<std::uint8_t>((2 * palette[0].g + palette[1].g) / 3),
			static_cast<std::uint8_t>((2 * palette[0].b + palette[1].b) / 3),
			255,
		};
		palette[3] = {
			static_cast<std::uint8_t>((2 * palette[1].r + palette[0].r) / 3),
			static_cast<std::uint8_t>((2 * palette[1].g + palette[0].g) / 3),
			static_cast<std::uint8_t>((2 * palette[1].b + palette[0].b) / 3),
			255,
		};
	}
	else
	{
		palette[2] = {
			static_cast<std::uint8_t>((palette[0].r + palette[1].r) / 2),
			static_cast<std::uint8_t>((palette[0].g + palette[1].g) / 2),
			static_cast<std::uint8_t>((palette[0].b + palette[1].b) / 2),
			255,
		};
		palette[3] = { 0, 0, 0, 0 };
	}

	const std::uint32_t indices = static_cast<std::uint32_t>(block[4])
		| (static_cast<std::uint32_t>(block[5]) << 8)
		| (static_cast<std::uint32_t>(block[6]) << 16)
		| (static_cast<std::uint32_t>(block[7]) << 24);

	for (int i = 0; i < 16; ++i)
	{
		out[i] = palette[(indices >> (i * 2)) & 0x3];
	}
}

void DecodeAlphaDxt3(const std::uint8_t* alphaBlock, std::uint8_t out[16])
{
	for (int i = 0; i < 16; ++i)
	{
		const std::uint8_t nibble = (alphaBlock[i / 2] >> ((i % 2) * 4)) & 0x0F;
		out[i] = static_cast<std::uint8_t>(nibble * 17);
	}
}

void DecodeAlphaDxt5(const std::uint8_t* alphaBlock, std::uint8_t out[16])
{
	const std::uint8_t a0 = alphaBlock[0];
	const std::uint8_t a1 = alphaBlock[1];

	std::uint8_t alphas[8];
	alphas[0] = a0;
	alphas[1] = a1;

	if (a0 > a1)
	{
		for (int i = 1; i <= 6; ++i)
		{
			alphas[i + 1] = static_cast<std::uint8_t>(((6 - i) * a0 + i * a1) / 7);
		}
	}
	else
	{
		for (int i = 1; i <= 4; ++i)
		{
			alphas[i + 1] = static_cast<std::uint8_t>(((5 - i) * a0 + i * a1) / 5);
		}
		alphas[6] = 0;
		alphas[7] = 255;
	}

	std::uint64_t indices = 0;
	for (int i = 0; i < 6; ++i)
	{
		indices |= static_cast<std::uint64_t>(alphaBlock[2 + i]) << (i * 8);
	}

	for (int i = 0; i < 16; ++i)
	{
		out[i] = alphas[(indices >> (i * 3)) & 0x7];
	}
}

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

	const bool dxt1 = format == re::render::TextureFormat::DXT1;

	std::size_t offset = 0;
	for (std::uint32_t by = 0; by < blockHeight; ++by)
	{
		for (std::uint32_t bx = 0; bx < blockWidth; ++bx)
		{
			const std::uint8_t* block = data.data() + offset;
			offset += blockSize;

			RgbaPixel colors[16];
			DecodeColorBlock(block + (dxt1 ? 0 : 8), colors);

			std::uint8_t alphas[16];
			if (format == re::render::TextureFormat::DXT3)
			{
				DecodeAlphaDxt3(block, alphas);
			}
			else if (format == re::render::TextureFormat::DXT5)
			{
				DecodeAlphaDxt5(block, alphas);
			}
			else if (!dxt1)
			{
				for (std::uint8_t& alpha : alphas)
				{
					alpha = 255;
				}
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

					const int index = py * 4 + px;
					auto [r, g, b, a] = colors[index];
					if (!dxt1)
					{
						a = alphas[index];
					}

					std::uint8_t* dst = &result[(static_cast<std::size_t>(y) * width + x) * 4];
					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = a;
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
	else
	{
		// Можно залогировать предупреждение или создать 1x1 текстуру-заглушку (magenta/checkerboard)
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

	m_filePath = filePath.ToString();
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