#pragma once

#include <Core/Assert.hpp>

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
	[[nodiscard]] constexpr Vector3 Cross(Vector3 const& rhs) const;

	[[nodiscard]] constexpr T Dot(Vector3 const& rhs) const;

	[[nodiscard]] constexpr T LengthSq() const;

	[[nodiscard]] auto Length() const;

	[[nodiscard]] Vector3 Normalized() const
		requires std::floating_point<T>;

	[[nodiscard]] Vector3& Normalize() const
		requires std::floating_point<T>;

	template <typename U>
	constexpr explicit operator Vector3<U>() const;

	[[nodiscard]] constexpr Vector3 operator-() const;

	constexpr auto operator<=>(Vector3 const&) const = default;

	T x{};
	T y{};
	T z{};
};

using Vector3i = Vector3<int>;
using Vector3u = Vector3<unsigned int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator+(Vector3<T> const& left, Vector3<T> const& right);

template <typename T>
constexpr Vector3<T>& operator+=(Vector3<T>& left, Vector3<T> const& right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator-(Vector3<T> const& left, Vector3<T> const& right);

template <typename T>
constexpr Vector3<T>& operator-=(Vector3<T>& left, Vector3<T> const& right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(Vector3<T> const& left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(T left, Vector3<T> const& right);

template <typename T>
constexpr Vector3<T>& operator*=(Vector3<T>& left, T right);

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator/(Vector3<T> const& left, T right);

template <typename T>
constexpr Vector3<T>& operator/=(Vector3<T>& left, T right);

template <typename T, std::floating_point U = T>
[[nodiscard]] constexpr Vector3<T> Lerp(Vector3<T> const& a, Vector3<T> const& b, U t);

} // namespace re

#include <Core/Math/Vector3.inl>