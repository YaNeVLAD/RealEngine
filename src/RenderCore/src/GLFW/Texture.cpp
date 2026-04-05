#include <RenderCore/GLFW/Texture.hpp>

#include <Core/Math/Color.hpp>
#include <RenderCore/IRenderAPI.hpp>
#include <RenderCore/LoadTexture.hpp>

#include <glad/glad.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

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
	using namespace re::render;

	const auto res = LoadTexture(filePath);
	if (!res)
	{
		return false;
	}
	const auto& [width, height, mipMapCount, format, buffer] = res.value();

	m_width = width;
	m_height = height;

	if (m_rendererID == 0)
	{
		glGenTextures(1, &m_rendererID);
	}

	glBindTexture(GL_TEXTURE_2D, m_rendererID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount > 0 ? mipMapCount - 1 : 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (format == TextureFormat::DXT1 || format == TextureFormat::DXT3 || format == TextureFormat::DXT5)
	{
		GLenum glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		unsigned int blockSize = 8;

		if (format == TextureFormat::DXT3)
		{
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			blockSize = 16;
		}
		else if (format == TextureFormat::DXT5)
		{
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			blockSize = 16;
		}

		unsigned int w = m_width, h = m_height, offset = 0;
		for (unsigned int level = 0; level < mipMapCount && (w || h); ++level)
		{
			w = std::max(w, 1u);
			h = std::max(h, 1u);
			const unsigned int size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;

			if (offset + size > buffer.size())
			{
				break;
			}

			glCompressedTexImage2D(
				GL_TEXTURE_2D, level, glFormat,
				w, h, 0,
				static_cast<int>(size), buffer.data() + offset);

			offset += size;
			w /= 2;
			h /= 2;
		}
	}
	else
	{
		const GLenum glFormat = (format == TextureFormat::RGB) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			static_cast<int>(m_width), static_cast<int>(m_height),
			0, glFormat, GL_UNSIGNED_BYTE, buffer.data());

		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}

	// GLfloat maxAnisotropy;
	// glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

	return true;
}

} // namespace re