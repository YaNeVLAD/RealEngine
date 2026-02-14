#pragma once

#include <Core/Hash.hpp>

#include <compare>
#include <string_view>

namespace re
{

namespace detail
{

template <typename TChar>
struct BaseHashedString
{
	using ValueType = TChar;
	using SizeType = std::size_t;
	using HashType = Hash_t;

	const ValueType* str;
	SizeType length;
	HashType hash;
};

} // namespace detail

template <typename TChar>
class BaseHashedString : public detail::BaseHashedString<TChar>
{
	using Base = detail::BaseHashedString<TChar>;
	using Params = hash::FNV_1a_Params<>;

	[[nodiscard]] static constexpr auto hasher(const std::basic_string_view<TChar>& sv) noexcept
	{
		Base base{ sv.data(), sv.size(), Params::offset };

		for (auto&& ch : sv)
		{
			base.hash = (base.hash ^ static_cast<typename Base::HashType>(ch)) * Params::prime;
		}

		return base;
	}

public:
	using ValueType = typename Base::ValueType;
	using SizeType = typename Base::SizeType;
	using HashType = typename Base::HashType;

	constexpr BaseHashedString() noexcept
		: BaseHashedString{ nullptr, 0u }
	{
	}

	constexpr BaseHashedString(const ValueType* str, const SizeType length) noexcept
		: Base{ hasher({ str, length }) }
	{
	}

	template <std::size_t N>
	consteval BaseHashedString(const ValueType (&str)[N]) noexcept
		: Base{ hasher({ static_cast<const ValueType*>(str) }) }
	{
	}

	[[nodiscard]] static constexpr HashType Value(const ValueType* str, const SizeType length) noexcept
	{
		return BaseHashedString{ str, length };
	}

	template <std::size_t N>
	[[nodiscard]] static consteval HashType Value(const char (&str)[N]) noexcept
	{
		return BaseHashedString{ str };
	}

	[[nodiscard]] constexpr const ValueType* Data() const noexcept
	{
		return Base::str;
	}

	[[nodiscard]] constexpr SizeType Size() const noexcept
	{
		return Base::length;
	}

	[[nodiscard]] constexpr SizeType Value() const noexcept
	{
		return Base::hash;
	}

	[[nodiscard]] constexpr operator const ValueType*() const noexcept
	{
		return Base::str;
	}

	[[nodiscard]] constexpr operator HashType() const noexcept
	{
		return Base::hash;
	}

	constexpr bool operator==(const BaseHashedString& rhs) const noexcept
	{
		return Base::hash == rhs.hash;
	}

	constexpr auto operator<=>(const BaseHashedString& rhs) const noexcept
	{
		return Base::hash <=> rhs.hash;
	}
};

template <typename TChar>
BaseHashedString(const TChar* str, std::size_t len) -> BaseHashedString<TChar>;

template <typename TChar, std::size_t N>
BaseHashedString(const TChar (&str)[N]) -> BaseHashedString<TChar>;

using HashedString = BaseHashedString<char>;
using HashedWString = BaseHashedString<wchar_t>;
using HashedU8String = BaseHashedString<char8_t>;
using HashedU16String = BaseHashedString<char16_t>;
using HashedU32String = BaseHashedString<char32_t>;

inline namespace literals
{

[[nodiscard]] consteval HashedString operator"" _hs(const char* str, std::size_t size) noexcept
{
	return HashedString{ str, size };
}

[[nodiscard]] consteval HashedWString operator"" _hws(const wchar_t* str, const std::size_t size) noexcept
{
	return HashedWString{ str, size };
}

[[nodiscard]] consteval HashedU8String operator"" _h8s(const char8_t* str, const std::size_t size) noexcept
{
	return HashedU8String{ str, size };
}

[[nodiscard]] consteval HashedU16String operator"" _h16s(const char16_t* str, const std::size_t size) noexcept
{
	return HashedU16String{ str, size };
}

[[nodiscard]] consteval HashedU32String operator"" _h32s(const char32_t* str, const std::size_t size) noexcept
{
	return HashedU32String{ str, size };
}

} // namespace literals

} // namespace re

template <typename TChar>
struct std::hash<re::BaseHashedString<TChar>>
{
	std::size_t operator()(re::BaseHashedString<TChar> const& str) const noexcept
	{
		return static_cast<std::size_t>(str.Value());
	}
};