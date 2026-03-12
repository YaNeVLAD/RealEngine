#pragma once

#include <cassert>
#include <cmath>
#include <compare>
#include <concepts>
#include <numbers>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
struct Vector3
{
	[[nodiscard]] constexpr Vector3 Cross(const Vector3& rhs) const;

	[[nodiscard]] constexpr T Dot(const Vector3& rhs) const;

	[[nodiscard]] constexpr T LengthSq() const;

	[[nodiscard]] auto Length() const;

	[[nodiscard]] Vector3 Normalize() const
		requires std::floating_point<T>;

	constexpr auto operator<=>(const Vector3&) const = default;

	T x{};
	T y{};
	T z{};
};

using Vector3i = Vector3<int>;
using Vector3u = Vector3<unsigned int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator+(Vector3<T> left, Vector3<T> right);

template <typename T>
[[nodiscard]] constexpr Vector3<T>& operator+=(Vector3<T>& left, Vector3<T> right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator-(Vector3<T> left, Vector3<T> right);

template <typename T>
constexpr Vector3<T>& operator-=(Vector3<T>& left, Vector3<T> right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(Vector3<T> left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(T left, Vector3<T> right);

template <typename T>
[[nodiscard]] constexpr Vector3<T>& operator*=(Vector3<T>& left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator/(Vector3<T> left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T>& operator/=(Vector3<T>& left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> Lerp(const Vector3<T>& a, const Vector3<T>& b, float t);

} // namespace re

#include <Core/Math/Vector3.inl>