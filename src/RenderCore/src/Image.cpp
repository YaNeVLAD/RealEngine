#include <RenderCore/Image.hpp>

#include <stb_image.h>
#include <stb_image_resize.h>

#include <iostream>

namespace re
{

Image::Image(const std::uint32_t width, const std::uint32_t height, const Color fillColor)
{
	Resize(width, height, fillColor);
}

void Image::Resize(const std::uint32_t width, const std::uint32_t height, const Color fillColor)
{
	m_width = width;
	m_height = height;
	m_data.resize(width * height * 4);
	Fill(fillColor);
}

void Image::Scale(const uint32_t newWidth, const uint32_t newHeight)
{
	if (m_width == 0 || m_height == 0 || newWidth == 0 || newHeight == 0)
	{
		return;
	}
	if (m_width == newWidth && m_height == newHeight)
	{
		return;
	}

	std::vector<uint8_t> newData(newWidth * newHeight * sizeof(Color));

	int result = 0;

	result = stbir_resize_uint8(
		m_data.data(),
		static_cast<int>(m_width), static_cast<int>(m_height),
		0,
		newData.data(),
		static_cast<int>(newWidth), static_cast<int>(newHeight),
		0,
		sizeof(Color));

	if (result)
	{
		m_data = std::move(newData);
		m_width = newWidth;
		m_height = newHeight;
	}
}

void Image::SetPixel(const std::uint32_t x, const std::uint32_t y, const Color color)
{
	if (x >= m_width || y >= m_height)
	{
		return;
	}

	const size_t index = (y * m_width + x) * sizeof(Color);
	m_data[index + 0] = color.r;
	m_data[index + 1] = color.g;
	m_data[index + 2] = color.b;
	m_data[index + 3] = color.a;
}

Color Image::GetPixel(const std::uint32_t x, const std::uint32_t y) const
{
	if (x >= m_width || y >= m_height)
	{
		return Color::Transparent;
	}

	const size_t index = (y * m_width + x) * sizeof(Color);
	return Color{
		m_data[index + 0],
		m_data[index + 1],
		m_data[index + 2],
		m_data[index + 3]
	};
}

void Image::Fill(Color const& fillColor)
{
	if (m_data.empty())
	{
		return;
	}

	const uint8_t pixel[sizeof(Color)] = {
		static_cast<uint8_t>(fillColor.r),
		static_cast<uint8_t>(fillColor.g),
		static_cast<uint8_t>(fillColor.b),
		static_cast<uint8_t>(fillColor.a)
	};

	for (size_t i = 0; i < m_data.size(); i += 4)
	{
		memcpy(&m_data[i], pixel, 4);
	}
}

const std::uint8_t* Image::Data() const
{
	return m_data.data();
}

std::uint32_t Image::Width() const
{
	return m_width;
}

std::uint32_t Image::Height() const
{
	return m_height;
}

std::uint32_t Image::Size() const
{
	return m_width * m_height * sizeof(Color);
}

bool Image::IsEmpty() const
{
	return m_data.empty();
}

bool Image::LoadFromFile(std::string const& filePath)
{
	int w, h, channels;
	stbi_uc* pixels = stbi_load(
		filePath.c_str(),
		&w, &h, &channels,
		STBI_rgb_alpha);

	if (!pixels)
	{
		std::cerr << "Failed to load image: " << filePath << std::endl;
		return false;
	}

	Resize(w, h);
	std::memcpy(m_data.data(), pixels, w * h * sizeof(Color));

	stbi_image_free(pixels);

	return true;
}

} // namespace re