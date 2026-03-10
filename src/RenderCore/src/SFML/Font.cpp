#include <RenderCore/SFML/Font.hpp>

#include <iostream>

namespace re
{

bool Font::LoadFromFile(String const& filePath)
{
	if (const auto str = filePath.ToString(); !m_font.openFromFile(str))
	{
		std::cerr << "Failed to load font file: " << str << std::endl;
		return false;
	}

	return true;
}

sf::Font const& Font::GetSfFont() const
{
	return m_font;
}

} // namespace re