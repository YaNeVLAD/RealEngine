#include <Core/Math/Vector2.hpp>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
Vector2<T> Vector2<T>::Rotate(const float angle)
{
	const float rad = angle * (3.14f / 180.f);
	float cos = std::cos(rad);
	float sin = std::sin(rad);

	return Vector2{
		x * cos - y * sin,
		x * sin + y * cos
	};
}

template <typename T>
constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right)
{
	return Vector2<T>(left.x + right.x, left.y + right.y);
}

template <typename T>
constexpr Vector2<T>& operator+=(Vector2<T>& left, Vector2<T> right)
{
	left.x += right.x;
	left.y += right.y;

	return left;
}

template <typename T>
constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right)
{
	return Vector2<T>(left.x - right.x, left.y - right.y);
}

template <typename T>
constexpr Vector2<T>& operator-=(Vector2<T>& left, Vector2<T> right)
{
	left.x -= right.x;
	left.y -= right.y;

	return left;
}

template <typename T>
constexpr Vector2<T> operator*(Vector2<T> left, T right)
{
	return Vector2<T>(left.x * right, left.y * right);
}

template <typename T>
constexpr Vector2<T> operator*(T left, Vector2<T> right)
{
	return Vector2<T>(left * right.x, left * right.y);
}

template <typename T>
constexpr Vector2<T>& operator*=(Vector2<T>& left, T right)
{
	left.x *= right;
	left.y *= right;

	return left;
}

template <typename T>
constexpr Vector2<T> operator/(Vector2<T> left, T right)
{
	assert(right != 0 && "Vector2::operator/ cannot divide by 0");

	return Vector2<T>(left.x / right, left.y / right);
}

template <typename T>
constexpr Vector2<T>& operator/=(Vector2<T>& left, T right)
{
	assert(right != 0 && "Vector2::operator/= cannot divide by 0");
	left.x /= right;
	left.y /= right;

	return left;
}

} // namespace re