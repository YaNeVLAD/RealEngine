#include <RenderCore/Image.hpp>

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

void Image::SetPixel(const std::uint32_t x, const std::uint32_t y, const Color color)
{
	if (x >= m_width || y >= m_height)
	{
		return;
	}

	const size_t index = (y * m_width + x) * 4;
	m_data[index + 0] = color.r * 255;
	m_data[index + 1] = color.g * 255;
	m_data[index + 2] = color.b * 255;
	m_data[index + 3] = color.a * 255;
}

Color Image::GetPixel(const std::uint32_t x, const std::uint32_t y) const
{
	if (x >= m_width || y >= m_height)
	{
		return Color::Transparent;
	}

	const size_t index = (y * m_width + x) * 4;
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
		return;

	const uint8_t pixel[4] = {
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

bool Image::IsEmpty() const
{
	return m_data.empty();
}

} // namespace re