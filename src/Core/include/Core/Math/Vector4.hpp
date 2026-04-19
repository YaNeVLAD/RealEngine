#pragma once

namespace re
{

template <typename T>
struct Vector4
{
	constexpr Vector4() noexcept = default;

	constexpr explicit Vector4(T value) noexcept;
	constexpr Vector4(T x, T y, T z, T w) noexcept;

	constexpr bool operator==(const Vector4&) const = default;

	T x{};
	T y{};
	T z{};
	T w{};
};

using Vector4i = Vector4<int>;
using Vector4u = Vector4<unsigned int>;
using Vector4f = Vector4<float>;
using Vector4d = Vector4<double>;

} // namespace re

#include <Core/Math/Vector4.inl>