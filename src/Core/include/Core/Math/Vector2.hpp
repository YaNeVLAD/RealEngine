#pragma once

#include <Core/Assert.hpp>

#include <cmath>
#include <compare>
#include <concepts>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
struct Vector2
{
	constexpr Vector2() noexcept = default;

	explicit constexpr Vector2(T value) noexcept;
	constexpr Vector2(T x, T y) noexcept;

	constexpr Vector2(const Vector2& value) noexcept = default;
	constexpr Vector2& operator=(const Vector2& value) noexcept = default;

	constexpr Vector2(Vector2&& value) noexcept = default;
	constexpr Vector2& operator=(Vector2&& value) noexcept = default;

	Vector2 Rotate(float angle) const noexcept;

	T Dot(const Vector2& rhs) const noexcept;

	T Length() noexcept;

	template <typename U>
	constexpr explicit operator Vector2<U>() const noexcept;

	constexpr auto operator<=>(const Vector2&) const noexcept = default;

	T x{};
	T y{};
};

using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right) noexcept;

template <typename T>
constexpr Vector2<T>& operator+=(Vector2<T>& left, Vector2<T> right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right) noexcept;

template <typename T>
constexpr Vector2<T>& operator-=(Vector2<T>& left, Vector2<T> right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, T right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(T left, Vector2<T> right) noexcept;

template <typename T>
constexpr Vector2<T>& operator*=(Vector2<T>& left, T right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, T right) noexcept;

template <typename T>
constexpr Vector2<T>& operator/=(Vector2<T>& left, T right) noexcept;

} // namespace re

#include <Core/Math/Vector2.inl>