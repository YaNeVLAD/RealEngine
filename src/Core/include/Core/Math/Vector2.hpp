#pragma once

#include <cassert>
#include <cmath>
#include <compare>
#include <concepts>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
struct Vector2
{
	Vector2 Rotate(float angle);

	constexpr auto operator<=>(const Vector2&) const = default;

	T x{};
	T y{};
};

using Vector2i = Vector2<int>;

using Vector2u = Vector2<unsigned int>;

using Vector2f = Vector2<float>;

using Vector2d = Vector2<double>;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right);

template <typename T>
[[nodiscard]] constexpr Vector2<T>& operator+=(Vector2<T>& left, Vector2<T> right);

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right);

template <typename T>
constexpr Vector2<T>& operator-=(Vector2<T>& left, Vector2<T> right);

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, T right);

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(T left, Vector2<T> right);

template <typename T>
[[nodiscard]] constexpr Vector2<T>& operator*=(Vector2<T>& left, T right);

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, T right);

template <typename T>
[[nodiscard]] constexpr Vector2<T>& operator/=(Vector2<T>& left, T right);

} // namespace re

#include <Core/Math/Vector2.inl>