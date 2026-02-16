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
inline constexpr Color Color::Gray(128, 128, 128);
inline constexpr Color Color::Yellow(255, 255, 0);
inline constexpr Color Color::Magenta(255, 0, 255);
inline constexpr Color Color::Cyan(0, 255, 255);
inline constexpr Color Color::Brown(165, 42, 42);
inline constexpr Color Color::Orange(255, 165, 0);
inline constexpr Color Color::Pink(255, 192, 203);
inline constexpr Color Color::Purple(128, 0, 128);
inline constexpr Color Color::Silver(192, 192, 192);
inline constexpr Color Color::Gold(255, 215, 0);
inline constexpr Color Color::Lime(50, 205, 50);
inline constexpr Color Color::Maroon(128, 0, 0);
inline constexpr Color Color::Navy(0, 0, 128);
inline constexpr Color Color::Olive(128, 128, 0);
inline constexpr Color Color::Teal(0, 128, 128);
inline constexpr Color Color::Transparent(0, 0, 0, 0);

} // namespace re