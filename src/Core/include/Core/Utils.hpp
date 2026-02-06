#pragma once

#include <variant>

namespace re::utils
{

template <typename T, typename TVariant>
struct is_alternative_of;

template <typename T, typename... Ts>
struct is_alternative_of<T, std::variant<Ts...>>
	: std::disjunction<std::is_same<T, Ts>...>
{
};

template <typename T, typename... Ts>
concept is_alternative_of_v = is_alternative_of<T, Ts...>::value;

template <typename... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};

} // namespace re::utils