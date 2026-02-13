#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Math/Color.hpp>

#include <cstdint>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API Image
{
public:
	Image() = default;
	Image(std::uint32_t width, std::uint32_t height, Color fillColor = Color::Transparent);

	void Resize(std::uint32_t width, std::uint32_t height, Color fillColor = Color::Transparent);

	void SetPixel(std::uint32_t x, std::uint32_t y, Color color);

	[[nodiscard]] Color GetPixel(std::uint32_t x, std::uint32_t y) const;

	void Fill(Color const& fillColor);

	[[nodiscard]] const std::uint8_t* Data() const;

	[[nodiscard]] std::uint32_t Width() const;
	[[nodiscard]] std::uint32_t Height() const;
	[[nodiscard]] std::uint32_t Size() const;

	[[nodiscard]] bool IsEmpty() const;

private:
	std::uint32_t m_width = 0;
	std::uint32_t m_height = 0;

	std::vector<std::uint8_t> m_data;
};

} // namespace re