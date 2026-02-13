#pragma once

#include <RenderCore/Export.hpp>

#include <cstdint>
#include <memory>

namespace sf
{
class Texture;
}

namespace re
{

class RE_RENDER_CORE_API Texture
{
public:
	Texture(std::uint32_t width, std::uint32_t height);

	void SetData(void* data, std::uint32_t size);

	[[nodiscard]] std::uint32_t Width() const;
	[[nodiscard]] std::uint32_t Height() const;

	[[nodiscard]] void* GetNativeHandle() const;

private:
	std::uint32_t m_width, m_height;
	std::unique_ptr<sf::Texture> m_texture;
};

} // namespace re