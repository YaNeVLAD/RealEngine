#pragma once

#include <compare>
#include <cstdint>
#include <tuple>

namespace re
{

struct ColorF;

class Color
{
public:
	constexpr Color() = default;

	constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255);

	constexpr explicit Color(std::uint32_t color);

	[[nodiscard]] constexpr Color WithR(std::uint8_t val) const;
	[[nodiscard]] constexpr Color WithG(std::uint8_t val) const;
	[[nodiscard]] constexpr Color WithB(std::uint8_t val) const;
	[[nodiscard]] constexpr Color WithA(std::uint8_t val) const;

	[[nodiscard]] constexpr std::uint32_t ToInt() const;
	[[nodiscard]] constexpr ColorF ToFloat() const;
	[[nodiscard]] static constexpr Color FromFloat(const ColorF& fColor);

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

	constexpr bool operator==(const Color&) const = default;
	constexpr bool operator!=(const Color&) const = default;
	constexpr auto operator<=>(const Color&) const = default;

	std::uint8_t r{};
	std::uint8_t g{};
	std::uint8_t b{};
	std::uint8_t a{ 255 };
};

struct ColorF
{
	constexpr ColorF();

	constexpr ColorF(float r, float g, float b, float a = 1.0f);

	[[nodiscard]] float* Data();

	[[nodiscard]] const float* Data() const;

	float r{};
	float g{};
	float b{};
	float a{ 1.f };
};

} // namespace re

#include <Core/Math/Color.inl>