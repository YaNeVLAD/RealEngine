#include <RenderCore/Filament/Font.hpp>

#include <cstring>
#include <fstream>

namespace re
{

Font::Font(const std::string& filepath)
{
	LoadFromFile(filepath);
}

Font::Font(const void* data, std::size_t size)
{
	LoadFromMemory(data, size);
}

bool Font::LoadFromFile(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return false;
	}

	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (size > 0)
	{
		m_fontData.resize(static_cast<std::size_t>(size));
		if (file.read(reinterpret_cast<char*>(m_fontData.data()), size))
		{
			return true;
		}
	}

	m_fontData.clear();
	return false;
}

bool Font::LoadFromMemory(const void* data, std::size_t size)
{
	if (data && size > 0)
	{
		m_fontData.resize(size);
		std::memcpy(m_fontData.data(), data, size);
		return true;
	}

	m_fontData.clear();
	return false;
}

} // namespace re