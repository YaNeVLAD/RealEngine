#include <Core/Math/Vector4.hpp>

namespace re
{

template <typename T>
constexpr Vector4<T>::Vector4(T value) noexcept
	: x(value)
	, y(value)
	, z(value)
	, w(value)
{
}

template <typename T>
constexpr Vector4<T>::Vector4(T x, T y, T z, T w) noexcept
	: x(x)
	, y(y)
	, z(z)
	, w(w)
{
}

} // namespace re