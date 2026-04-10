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
	constexpr Vector3() noexcept = default;

	constexpr explicit Vector3(T value) noexcept;
	constexpr Vector3(T x, T y, T z) noexcept;

	constexpr Vector3(const Vector3& other) noexcept = default;
	constexpr Vector3& operator=(const Vector3& other) noexcept = default;

	constexpr Vector3(Vector3&& other) noexcept = default;
	constexpr Vector3& operator=(Vector3&& other) noexcept = default;

	[[nodiscard]] constexpr Vector3 Cross(Vector3 const& rhs) const noexcept;

	[[nodiscard]] constexpr T Dot(Vector3 const& rhs) const noexcept;

	[[nodiscard]] constexpr T LengthSq() const noexcept;

	[[nodiscard]] auto Length() const noexcept;

	[[nodiscard]] Vector3 Normalized() const noexcept
		requires std::floating_point<T>;

	[[nodiscard]] Vector3& Normalize() noexcept
		requires std::floating_point<T>;

	[[nodiscard]] static constexpr Vector3 Zero() noexcept;

	[[nodiscard]] static constexpr Vector3 Up() noexcept;
	[[nodiscard]] static constexpr Vector3 Down() noexcept;
	[[nodiscard]] static constexpr Vector3 Left() noexcept;
	[[nodiscard]] static constexpr Vector3 Right() noexcept;
	[[nodiscard]] static constexpr Vector3 Forward() noexcept;
	[[nodiscard]] static constexpr Vector3 Backward() noexcept;

	template <typename U>
	constexpr explicit operator Vector3<U>() const noexcept;

	[[nodiscard]] constexpr Vector3 operator-() const noexcept;

	constexpr auto operator<=>(Vector3 const&) const noexcept = default;

	T x{};
	T y{};
	T z{};
};

using Vector3i = Vector3<int>;
using Vector3u = Vector3<unsigned int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator+(Vector3<T> const& left, Vector3<T> const& right) noexcept;

template <typename T>
constexpr Vector3<T>& operator+=(Vector3<T>& left, Vector3<T> const& right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator-(Vector3<T> const& left, Vector3<T> const& right) noexcept;

template <typename T>
constexpr Vector3<T>& operator-=(Vector3<T>& left, Vector3<T> const& right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(Vector3<T> const& left, T right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(T left, Vector3<T> const& right) noexcept;

template <typename T>
constexpr Vector3<T>& operator*=(Vector3<T>& left, T right) noexcept;

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator/(Vector3<T> const& left, T right) noexcept;

template <typename T>
constexpr Vector3<T>& operator/=(Vector3<T>& left, T right) noexcept;

template <typename T, std::floating_point U = T>
[[nodiscard]] constexpr Vector3<T> Lerp(Vector3<T> const& a, Vector3<T> const& b, U t) noexcept;

} // namespace re

#include <Core/Math/Vector3.inl>