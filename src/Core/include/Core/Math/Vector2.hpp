#pragma once

#include <cmath>
#include <concepts>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
struct Vector2
{
	T x{};
	T y{};

	Vector2 Rotate(const float angle)
	{
		const float rad = angle * (3.14f / 180.f);
		float cos = std::cos(rad);
		float sin = std::sin(rad);

		return Vector2{
			x * cos - y * sin,
			x * sin + y * cos
		};
	}
};

using Vector2i = Vector2<int>;

using Vector2u = Vector2<unsigned int>;

using Vector2f = Vector2<float>;

using Vector2d = Vector2<double>;

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right)
{
	return Vector2<T>(left.x + right.x, left.y + right.y);
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right)
{
	return Vector2<T>(left.x - right.x, left.y - right.y);
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, T right)
{
	return Vector2<T>(left.x * right, left.y * right);
}

} // namespace re