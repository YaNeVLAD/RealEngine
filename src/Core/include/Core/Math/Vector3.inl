#include <Core/Math/Vector3.hpp>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Cross(const Vector3& rhs) const noexcept
{
	return Vector3{
		y * rhs.z - z * rhs.y,
		z * rhs.x - x * rhs.z,
		x * rhs.y - y * rhs.x
	};
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr T Vector3<T>::Dot(const Vector3& rhs) const noexcept
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr T Vector3<T>::LengthSq() const noexcept
{
	return x * x + y * y + z * z;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
template <typename U>
constexpr Vector3<T>::operator Vector3<U>() const noexcept
{
	return Vector3<U>{ static_cast<U>(x), static_cast<U>(y), static_cast<U>(z) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::operator-() const noexcept
{
	return Vector3{ -x, -y, -z };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T>::Vector3(T value) noexcept
	: x(value)
	, y(value)
	, z(value)
{
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T>::Vector3(T x, T y, T z) noexcept
	: x(x)
	, y(y)
	, z(z)
{
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
auto Vector3<T>::Length() const noexcept
{
	return std::sqrt(LengthSq());
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
		 Vector3<T> Vector3<T>::Normalized() const noexcept
	requires std::floating_point<T>
{
	const T len = Length();
	RE_ASSERT(len != 0, "Vector3::Normalize cannot divide by 0");

	return *this / len;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
		 Vector3<T>& Vector3<T>::Normalize() noexcept
	requires std::floating_point<T>
{
	const T len = Length();
	RE_ASSERT(len > 0, "Vector3::Normalize cannot divide by 0");

	const T invLen = static_cast<T>(1) / len;

	x *= invLen;
	y *= invLen;
	z *= invLen;

	return *this;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Zero() noexcept
{
	return Vector3{ static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Up() noexcept
{
	return Vector3{ static_cast<T>(0), static_cast<T>(1), static_cast<T>(0) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Down() noexcept
{
	return Vector3{ static_cast<T>(0), static_cast<T>(-1), static_cast<T>(0) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Left() noexcept
{
	return Vector3{ static_cast<T>(-1), static_cast<T>(0), static_cast<T>(0) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Right() noexcept
{
	return Vector3{ static_cast<T>(1), static_cast<T>(0), static_cast<T>(0) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Forward() noexcept
{
	return Vector3{ static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Backward() noexcept
{
	return Vector3{ static_cast<T>(0), static_cast<T>(0), static_cast<T>(-1) };
}

template <typename T>
constexpr Vector3<T> operator+(Vector3<T> const& left, Vector3<T> const& right) noexcept
{
	return Vector3<T>{ left.x + right.x, left.y + right.y, left.z + right.z };
}

template <typename T>
constexpr Vector3<T>& operator+=(Vector3<T>& left, Vector3<T> const& right) noexcept
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;

	return left;
}

template <typename T>
constexpr Vector3<T> operator-(Vector3<T> const& left, Vector3<T> const& right) noexcept
{
	return Vector3<T>{ left.x - right.x, left.y - right.y, left.z - right.z };
}

template <typename T>
constexpr Vector3<T>& operator-=(Vector3<T>& left, Vector3<T> const& right) noexcept
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;

	return left;
}

template <typename T>
constexpr Vector3<T> operator*(Vector3<T> const& left, T right) noexcept
{
	return Vector3<T>{ left.x * right, left.y * right, left.z * right };
}

template <typename T>
constexpr Vector3<T> operator*(T left, Vector3<T> const& right) noexcept
{
	return Vector3<T>{ left * right.x, left * right.y, left * right.z };
}

template <typename T>
constexpr Vector3<T> operator*(Vector3<T> const& left, Vector3<T> const& right) noexcept
{
	return Vector3<T>{ left.x * right.x, left.y * right.y, left.z * right.z };
}

template <typename T>
constexpr Vector3<T>& operator*=(Vector3<T>& left, Vector3<T> const& right) noexcept
{
	left.x *= right.x;
	left.y *= right.y;
	left.z *= right.z;

	return left;
}

template <typename T>
constexpr Vector3<T> operator/(Vector3<T> const& left, Vector3<T> const& right) noexcept
{
	constexpr static auto ZERO = static_cast<T>(0);
	RE_ASSERT(right.x != ZERO && right.y != ZERO && right.z != ZERO, "Vector3::operator/= cannot divide by 0");

	return Vector3<T>{ left.x / right.x, left.y / right.y, left.z / right.z };
}

template <typename T>
constexpr Vector3<T>& operator/=(Vector3<T>& left, Vector3<T> const& right) noexcept
{
	constexpr static auto ZERO = static_cast<T>(0);
	RE_ASSERT(right.x != ZERO && right.y != ZERO && right.z != ZERO, "Vector3::operator/= cannot divide by 0");

	left.x /= right.x;
	left.y /= right.y;
	left.z /= right.z;

	return left;
}

template <typename T>
constexpr Vector3<T>& operator*=(Vector3<T>& left, T right) noexcept
{
	left.x *= right;
	left.y *= right;
	left.z *= right;

	return left;
}

template <typename T>
constexpr Vector3<T> operator/(Vector3<T> const& left, T right) noexcept
{
	RE_ASSERT(right != static_cast<T>(0), "Vector3::operator/ cannot divide by 0");

	return Vector3<T>{ left.x / right, left.y / right, left.z / right };
}

template <typename T>
constexpr Vector3<T>& operator/=(Vector3<T>& left, T right) noexcept
{
	RE_ASSERT(right != static_cast<T>(0), "Vector3::operator/= cannot divide by 0");

	left.x /= right;
	left.y /= right;
	left.z /= right;

	return left;
}

template <typename T, std::floating_point U>
constexpr Vector3<T> Lerp(Vector3<T> const& a, Vector3<T> const& b, U t) noexcept
{
	return a * (static_cast<U>(1) - t) + b * t;
}

} // namespace re