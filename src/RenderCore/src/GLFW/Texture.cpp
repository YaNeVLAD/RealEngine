#include <RenderCore/GLFW/Texture.hpp>

#include <Core/Math/Color.hpp>
#include <RenderCore/Image.hpp>

#include <glad/glad.h>

#include <iostream>

namespace re
{

Texture::Texture(const std::uint32_t width, const std::uint32_t height)
	: m_width(width)
	, m_height(height)
{
	glGenTextures(1, &m_rendererID);
	glBindTexture(GL_TEXTURE_2D, m_rendererID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		static_cast<int>(width), static_cast<int>(height), 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_rendererID);
}

void Texture::SetData(const std::uint8_t* data, const std::uint32_t size)
{
	if (size != m_width * m_height * sizeof(Color))
	{
		std::cerr << "Texture::SetData: Invalid size provided!" << std::endl;
		return;
	}

	glBindTexture(GL_TEXTURE_2D, m_rendererID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,
		0, static_cast<int>(m_width), static_cast<int>(m_height),
		GL_RGBA, GL_UNSIGNED_BYTE, data);
}

std::uint32_t Texture::Width() const
{
	return m_width;
}

std::uint32_t Texture::Height() const
{
	return m_height;
}

std::uint32_t Texture::Size() const
{
	return m_width * m_height * sizeof(Color);
}

void* Texture::GetNativeHandle()
{
	return &m_rendererID;
}

bool Texture::LoadFromFile(String const& filePath)
{
	Image tempImage;
	if (!tempImage.LoadFromFile(filePath))
	{
		return false;
	}

	m_width = tempImage.Width();
	m_height = tempImage.Height();

	if (m_rendererID == 0)
	{
		glGenTextures(1, &m_rendererID);
		glBindTexture(GL_TEXTURE_2D, m_rendererID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_rendererID);
	}

	GLenum format = GL_RGBA;
	if (tempImage.Channels() == 3)
	{
		format = GL_RGB;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		static_cast<int>(m_width), static_cast<int>(m_height), 0,
		format, GL_UNSIGNED_BYTE, tempImage.Data());

	return true;
}

} // namespace re