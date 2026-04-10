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

template <typename T, typename TPtr = std::shared_ptr<T>>
struct Prototype
{
	using ReturnType = TPtr;

	virtual ~Prototype() = default;

	virtual ReturnType Clone() const = 0;
};

template <typename TDerived, typename TBase>
	requires std::derived_from<TBase, Prototype<TBase>>
struct Clonable : TBase
{
	using TBase::TBase;

	typename TBase::ReturnType Clone() const override
	{
		return std::make_shared<TDerived>(static_cast<const TDerived&>(*this));
	}
};

} // namespace re::utils