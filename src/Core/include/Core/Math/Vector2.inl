#include <Core/Math/Vector2.hpp>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
Vector2<T> Vector2<T>::Rotate(const float angle) const noexcept
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
	requires std::floating_point<T> || std::integral<T>
T Vector2<T>::Dot(const Vector2& rhs) const noexcept
{
	return x * rhs.x + y * rhs.y;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
T Vector2<T>::Length() noexcept
{
	return std::sqrt(x * x + y * y);
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector2<T>::Vector2(T value) noexcept
	: x(value)
	, y(value)
{
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector2<T>::Vector2(T x, T y) noexcept
	: x(x)
	, y(y)
{
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
template <typename U>
constexpr Vector2<T>::operator Vector2<U>() const noexcept
{
	return Vector2<U>{ static_cast<U>(x), static_cast<U>(y) };
}

template <typename T>
constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right) noexcept
{
	return Vector2<T>(left.x + right.x, left.y + right.y);
}

template <typename T>
constexpr Vector2<T>& operator+=(Vector2<T>& left, Vector2<T> right) noexcept
{
	left.x += right.x;
	left.y += right.y;

	return left;
}

template <typename T>
constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right) noexcept
{
	return Vector2<T>(left.x - right.x, left.y - right.y);
}

template <typename T>
constexpr Vector2<T>& operator-=(Vector2<T>& left, Vector2<T> right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;

	return left;
}

template <typename T>
constexpr Vector2<T> operator*(Vector2<T> left, T right) noexcept
{
	return Vector2<T>(left.x * right, left.y * right);
}

template <typename T>
constexpr Vector2<T> operator*(T left, Vector2<T> right) noexcept
{
	return Vector2<T>(left * right.x, left * right.y);
}

template <typename T>
constexpr Vector2<T>& operator*=(Vector2<T>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;

	return left;
}

template <typename T>
constexpr Vector2<T> operator/(Vector2<T> left, T right) noexcept
{
	RE_ASSERT(right != 0, "Vector2::operator/ cannot divide by 0");

	return Vector2<T>(left.x / right, left.y / right);
}

template <typename T>
constexpr Vector2<T>& operator/=(Vector2<T>& left, T right) noexcept
{
	RE_ASSERT(right != 0, "Vector2::operator/= cannot divide by 0");
	left.x /= right;
	left.y /= right;

	return left;
}

} // namespace re