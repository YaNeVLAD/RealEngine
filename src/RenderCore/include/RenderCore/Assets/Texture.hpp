#pragma once

#include <RenderCore/Assets/IAsset.hpp>
#include <RenderCore/Export.hpp>

#include <cstdint>
#include <memory>

namespace sf
{
class Texture;
}

namespace re
{

class RE_RENDER_CORE_API Texture final : public IAsset
{
public:
	Texture() = default;

	Texture(std::uint32_t width, std::uint32_t height);

	void SetData(const std::uint8_t* data, std::uint32_t size);

	[[nodiscard]] std::uint32_t Width() const;
	[[nodiscard]] std::uint32_t Height() const;

	[[nodiscard]] void* GetNativeHandle() const;

	bool LoadFromFile(std::string const& filePath) override;

private:
	std::uint32_t m_width{};
	std::uint32_t m_height{};

	std::unique_ptr<sf::Texture> m_texture;
};

} // namespace re