#include <RenderCore/SFML/Font.hpp>

#include <iostream>

namespace re
{

bool Font::LoadFromFile(std::string const& filePath)
{
	if (!m_font.openFromFile(filePath))
	{
		std::cerr << "Failed to load font file: " << filePath << std::endl;
		return false;
	}

	return true;
}

sf::Font const& Font::GetSfFont() const
{
	return m_font;
}

} // namespace re