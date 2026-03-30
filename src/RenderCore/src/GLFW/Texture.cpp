#include <RenderCore/GLFW/Texture.hpp>

#include <Core/Math/Color.hpp>
#include <RenderCore/Image.hpp>

#include <glad/glad.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

namespace
{

bool LoadDDS(const std::string& pathStr, const std::uint32_t rendererID, std::uint32_t& outWidth, std::uint32_t& outHeight)
{
	std::ifstream file(pathStr, std::ios::binary);
	if (!file)
	{
		std::cerr << "[Texture Error]: Failed to open DDS file: " << pathStr << std::endl;
		return false;
	}

	char magic[4];
	file.read(magic, 4);
	if (strncmp(magic, "DDS ", 4) != 0)
	{
		std::cerr << "[Texture Error]: Not a valid DDS file: " << pathStr << std::endl;
		return false;
	}

	unsigned char header[124];
	file.read(reinterpret_cast<char*>(header), 124);

	outHeight = *reinterpret_cast<unsigned int*>(&(header[8]));
	outWidth = *reinterpret_cast<unsigned int*>(&(header[12]));
	const auto mipMapCount = *reinterpret_cast<unsigned int*>(&(header[24]));
	const auto fourCC = *reinterpret_cast<unsigned int*>(&(header[80]));

	GLenum format;
	constexpr unsigned int FOURCC_DXT1 = 0x31545844; // "DXT1" в ASCII
	constexpr unsigned int FOURCC_DXT3 = 0x33545844; // "DXT3" в ASCII
	constexpr unsigned int FOURCC_DXT5 = 0x35545844; // "DXT5" в ASCII

	if (fourCC == FOURCC_DXT1)
	{
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	}
	else if (fourCC == FOURCC_DXT3)
	{
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	}
	else if (fourCC == FOURCC_DXT5)
	{
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	else
	{
		std::cerr << "[Texture Error]: Unsupported DDS format in " << pathStr << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	const std::streamsize length = file.tellg();
	file.seekg(128, std::ios::beg);

	std::vector<unsigned char> buffer(static_cast<std::size_t>(length) - 128);
	file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

	const unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int offset = 0;

	glBindTexture(GL_TEXTURE_2D, rendererID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount > 0 ? mipMapCount - 1 : 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	int w = static_cast<int>(outWidth);
	int h = static_cast<int>(outHeight);

	for (int level = 0; level < mipMapCount && (w || h); ++level)
	{
		w = std::max(w, 1);
		h = std::max(h, 1);

		const unsigned int size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;

		if (offset + size > buffer.size())
		{
			break;
		}

		glCompressedTexImage2D(
			GL_TEXTURE_2D,
			level,
			format,
			w,
			h,
			0,
			static_cast<int>(size),
			buffer.data() + offset);

		offset += size;
		w /= 2;
		h /= 2;
	}

	return true;
}

bool LoadStandardImage(const std::string& pathStr, const std::uint32_t rendererID, std::uint32_t& outWidth, std::uint32_t& outHeight)
{
	re::Image tempImage;
	if (!tempImage.LoadFromFile(pathStr))
	{
		std::cerr << "[Texture Error]: Failed to load standard image: " << pathStr << std::endl;
		return false;
	}

	outWidth = tempImage.Width();
	outHeight = tempImage.Height();

	glBindTexture(GL_TEXTURE_2D, rendererID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	const GLenum format = (tempImage.Channels() == 3) ? GL_RGB : GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		static_cast<int>(outWidth), static_cast<int>(outHeight), 0,
		format, GL_UNSIGNED_BYTE, tempImage.Data());

	return true;
}

} // namespace

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
	const std::string pathStr = filePath.ToString();

	std::string lowerPath = pathStr;
	std::ranges::transform(lowerPath, lowerPath.begin(), tolower);

	if (m_rendererID == 0)
	{
		glGenTextures(1, &m_rendererID);
	}

	if (lowerPath.length() >= 4 && lowerPath.substr(lowerPath.length() - 4) == ".dds")
	{
		return LoadDDS(pathStr, m_rendererID, m_width, m_height);
	}

	return LoadStandardImage(pathStr, m_rendererID, m_width, m_height);
}

} // namespace re