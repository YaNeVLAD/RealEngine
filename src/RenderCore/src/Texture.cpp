#include <RenderCore/Assets/Texture.hpp>

#include <Core/Math/Color.hpp>
#include <RenderCore/Image.hpp>

#include <SFML/Graphics/Texture.hpp>

namespace re
{

Texture::Texture(std::uint32_t width, std::uint32_t height)
	: m_width(width)
	, m_height(height)
	, m_texture(std::make_unique<sf::Texture>())
{
	std::ignore = m_texture->resize({ width, height });
}

void Texture::SetData(const std::uint8_t* data, const std::uint32_t size)
{
	if (size == m_width * m_height * sizeof(Color))
	{
		m_texture->update(data);
	}
}

std::uint32_t Texture::Width() const
{
	return m_width;
}

std::uint32_t Texture::Height() const
{
	return m_height;
}

void* Texture::GetNativeHandle() const
{
	return m_texture.get();
}

bool Texture::LoadFromFile(std::string const& filePath)
{
	Image tempImage;
	if (!tempImage.LoadFromFile(filePath))
	{
		return false;
	}

	SetData(tempImage.Data(), tempImage.Width() * tempImage.Height() * sizeof(Color));

	return true;
}

} // namespace re