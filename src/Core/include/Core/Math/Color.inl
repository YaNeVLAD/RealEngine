#include <Core/Math/Color.hpp>

namespace re
{

constexpr Color::Color(
	const uint8_t r,
	const uint8_t g,
	const uint8_t b,
	const uint8_t a)
	: r(r)
	, g(g)
	, b(b)
	, a(a)
{
}

constexpr Color::Color(const uint32_t color)
	: r(static_cast<uint8_t>((color & 0xff000000) >> 24))
	, g(static_cast<uint8_t>((color & 0x00ff0000) >> 16))
	, b(static_cast<uint8_t>((color & 0x0000ff00) >> 8))
	, a(static_cast<uint8_t>(color & 0x000000ff))
{
}

constexpr uint32_t Color::ToInt() const
{
	return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) | a);
}

constexpr bool operator==(const Color left, const Color right)
{
	return (left.r == right.r) && (left.g == right.g) && (left.b == right.b) && (left.a == right.a);
}

constexpr bool operator!=(const Color left, const Color right)
{
	return !(left == right);
}

inline constexpr Color Color::Black(0, 0, 0);
inline constexpr Color Color::White(255, 255, 255);
inline constexpr Color Color::Red(255, 0, 0);
inline constexpr Color Color::Green(0, 255, 0);
inline constexpr Color Color::Blue(0, 0, 255);
inline constexpr Color Color::Yellow(255, 255, 0);
inline constexpr Color Color::Magenta(255, 0, 255);
inline constexpr Color Color::Cyan(0, 255, 255);
inline constexpr Color Color::Transparent(0, 0, 0, 0);

} // namespace re