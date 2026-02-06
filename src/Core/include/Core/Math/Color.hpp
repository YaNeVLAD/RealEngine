#pragma once

#include <compare>
#include <cstdint>

namespace re
{

class Color
{
public:
	constexpr Color() = default;

	constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

	constexpr explicit Color(uint32_t color);

	[[nodiscard]] constexpr uint32_t ToInt() const;

	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Magenta;
	static const Color Cyan;
	static const Color Transparent;

	uint8_t r{};
	uint8_t g{};
	uint8_t b{};
	uint8_t a{ 255 };
};

[[nodiscard]] constexpr bool operator==(Color left, Color right);

[[nodiscard]] constexpr bool operator!=(Color left, Color right);

} // namespace re

#include <Core/Math/Color.inl>