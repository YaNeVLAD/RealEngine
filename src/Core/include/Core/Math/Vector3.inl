#include <Core/Math/Vector3.hpp>

namespace re
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::Cross(const Vector3& rhs) const
{
	return Vector3{
		y * rhs.z - z * rhs.y,
		z * rhs.x - x * rhs.z,
		x * rhs.y - y * rhs.x
	};
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr T Vector3<T>::Dot(const Vector3& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr T Vector3<T>::LengthSq() const
{
	return x * x + y * y + z * z;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
template <typename U>
constexpr Vector3<T>::operator Vector3<U>() const
{
	return Vector3<U>{ static_cast<U>(x), static_cast<U>(y), static_cast<U>(z) };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
constexpr Vector3<T> Vector3<T>::operator-() const
{
	return Vector3{ -x, -y, -z };
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
auto Vector3<T>::Length() const
{
	return std::sqrt(LengthSq());
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
		 Vector3<T> Vector3<T>::Normalized() const
	requires std::floating_point<T>
{
	const T len = Length();
	RE_ASSERT(len != 0, "Vector3::Normalize cannot divide by 0");

	return *this / len;
}

template <typename T>
	requires std::floating_point<T> || std::integral<T>
		 Vector3<T>& Vector3<T>::Normalize() const
	requires std::floating_point<T>
{
	const T len = Length();
	RE_ASSERT(len != 0, "Vector3::Normalize cannot divide by 0");

	*this /= len;

	return *this;
}

template <typename T>
constexpr Vector3<T> operator+(Vector3<T> const& left, Vector3<T> const& right)
{
	return Vector3<T>{ left.x + right.x, left.y + right.y, left.z + right.z };
}

template <typename T>
constexpr Vector3<T>& operator+=(Vector3<T>& left, Vector3<T> const& right)
{
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;

	return left;
}

template <typename T>
constexpr Vector3<T> operator-(Vector3<T> const& left, Vector3<T> const& right)
{
	return Vector3<T>{ left.x - right.x, left.y - right.y, left.z - right.z };
}

template <typename T>
constexpr Vector3<T>& operator-=(Vector3<T>& left, Vector3<T> const& right)
{
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;

	return left;
}

template <typename T>
constexpr Vector3<T> operator*(Vector3<T> const& left, T right)
{
	return Vector3<T>{ left.x * right, left.y * right, left.z * right };
}

template <typename T>
constexpr Vector3<T> operator*(T left, Vector3<T> const& right)
{
	return Vector3<T>{ left * right.x, left * right.y, left * right.z };
}

template <typename T>
constexpr Vector3<T>& operator*=(Vector3<T>& left, T right)
{
	left.x *= right;
	left.y *= right;
	left.z *= right;

	return left;
}

template <typename T>
constexpr Vector3<T> operator/(Vector3<T> const& left, T right)
{
	RE_ASSERT(right != 0, "Vector3::operator/ cannot divide by 0");

	return Vector3<T>{ left.x / right, left.y / right, left.z / right };
}

template <typename T>
constexpr Vector3<T>& operator/=(Vector3<T>& left, T right)
{
	RE_ASSERT(right != 0, "Vector3::operator/= cannot divide by 0");

	left.x /= right;
	left.y /= right;
	left.z /= right;

	return left;
}

template <typename T, std::floating_point U>
constexpr Vector3<T> Lerp(Vector3<T> const& a, Vector3<T> const& b, U t)
{
	return a + (b - a) * t;
}

} // namespace re