#pragma once

#include <cstddef>
#include <cstdint>

namespace re
{
using Hash_t = std::size_t;

constexpr Hash_t INVALID_HASH = 0ull;

namespace hash
{

template <typename = Hash_t>
struct FNV_1a_Params;

template <>
struct FNV_1a_Params<std::uint32_t>
{
	static constexpr auto offset = 2166136261;
	static constexpr auto prime = 16777619;
};

template <>
struct FNV_1a_Params<std::uint64_t>
{
	static constexpr auto offset = 14695981039346656037ull;
	static constexpr auto prime = 1099511628211ull;
};

} // namespace hash

} // namespace re