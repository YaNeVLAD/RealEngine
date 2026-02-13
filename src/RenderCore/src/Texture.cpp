#include <RenderCore/Assets/Texture.hpp>

#include <SFML/Graphics/Texture.hpp>

namespace re
{

Texture::Texture(std::uint32_t width, std::uint32_t height)
	: m_width(width)
	, m_height(height)
{
	m_texture = std::make_unique<sf::Texture>();
	if (m_texture->resize({ width, height }))
	{
	}
}

void Texture::SetData(void* data, std::uint32_t size)
{
	m_texture->update(static_cast<const std::uint8_t*>(data));
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

} // namespace re