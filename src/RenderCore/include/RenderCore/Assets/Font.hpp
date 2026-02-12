#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

// TODO: Remove SFML dependency from RenderCore API
#include <SFML/Graphics/Font.hpp>

#include <string>

namespace re
{

class RE_RENDER_CORE_API Font final : public IAsset
{
public:
	bool LoadFromFile(std::string const& filePath) override;

	sf::Font const& GetSfFont() const;

private:
	sf::Font m_font;
};

} // namespace re