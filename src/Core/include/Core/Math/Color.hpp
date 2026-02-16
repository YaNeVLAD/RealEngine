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
	static const Color Gray;
	static const Color Yellow;
	static const Color Magenta;
	static const Color Cyan;
	static const Color Brown;
	static const Color Orange;
	static const Color Pink;
	static const Color Purple;
	static const Color Silver;
	static const Color Gold;
	static const Color Lime;
	static const Color Maroon;
	static const Color Navy;
	static const Color Olive;
	static const Color Teal;
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