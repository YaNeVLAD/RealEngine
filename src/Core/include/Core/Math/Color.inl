#include <Core/Math/Color.hpp>

namespace re
{

constexpr Color::Color(
	const std::uint8_t r,
	const std::uint8_t g,
	const std::uint8_t b,
	const std::uint8_t a)
	: r(r)
	, g(g)
	, b(b)
	, a(a)
{
}

constexpr Color::Color(const std::uint32_t color)
	: r(static_cast<std::uint8_t>((color & 0xff000000) >> 24))
	, g(static_cast<std::uint8_t>((color & 0x00ff0000) >> 16))
	, b(static_cast<std::uint8_t>((color & 0x0000ff00) >> 8))
	, a(static_cast<std::uint8_t>(color & 0x000000ff))
{
}

constexpr Color Color::WithR(const std::uint8_t val) const
{
	Color c = *this;
	c.r = val;

	return c;
}

constexpr Color Color::WithG(const std::uint8_t val) const
{
	Color c = *this;
	c.g = val;

	return c;
}

constexpr Color Color::WithB(const std::uint8_t val) const
{
	Color c = *this;
	c.b = val;

	return c;
}

constexpr Color Color::WithA(const std::uint8_t val) const
{
	Color c = *this;
	c.a = val;

	return c;
}

constexpr std::uint32_t Color::ToInt() const
{
	return static_cast<std::uint32_t>((r << 24) | (g << 16) | (b << 8) | a);
}

constexpr ColorF Color::ToFloat() const
{
	return ColorF{
		static_cast<float>(r) / 255.0f,
		static_cast<float>(g) / 255.0f,
		static_cast<float>(b) / 255.0f,
		static_cast<float>(a) / 255.0f,
	};
}

constexpr Color Color::FromFloat(const ColorF& fColor)
{
	return {
		static_cast<uint8_t>(fColor.r * 255.0f),
		static_cast<uint8_t>(fColor.g * 255.0f),
		static_cast<uint8_t>(fColor.b * 255.0f),
		static_cast<uint8_t>(fColor.a * 255.0f),
	};
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

constexpr ColorF::ColorF() = default;

constexpr ColorF::ColorF(const float r, const float g, const float b, const float a)
	: r(r)
	, g(g)
	, b(b)
	, a(a)
{
}

inline float* ColorF::Data()
{
	return &r;
}

inline const float* ColorF::Data() const
{
	return &r;
}

} // namespace re