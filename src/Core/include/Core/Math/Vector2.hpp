#pragma once

#include <concepts>

namespace re::core
{

template <typename T>
	requires std::floating_point<T> || std::integral<T>
struct Vector2
{
	T x{};
	T y{};
};

using Vector2i = Vector2<int>;

using Vector2u = Vector2<unsigned int>;

using Vector2f = Vector2<float>;

using Vector2d = Vector2<double>;

} // namespace re::core