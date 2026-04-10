#pragma once

#include <memory>
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

template <typename T>
struct Prototype
{
	virtual ~Prototype() = default;

	virtual std::shared_ptr<T> Clone() const = 0;
};

template <typename TDerived, typename TBase>
	requires std::derived_from<TBase, Prototype<TBase>>
struct SharedClone : TBase
{
	using TBase::TBase;

	std::shared_ptr<TBase> Clone() const override
	{
		return std::make_shared<TDerived>(static_cast<const TDerived&>(*this));
	}
};

} // namespace re::utils