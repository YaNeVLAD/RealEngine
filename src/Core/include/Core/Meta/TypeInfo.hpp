#pragma once

#include <Core/Export.hpp>

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace re::meta
{

using TypeHashType = std::size_t;
using TypeIndexType = std::size_t;

constexpr TypeHashType InvalidTypeHash = std::numeric_limits<TypeHashType>::max();

struct TypeInfo
{
	TypeHashType hash{};
	TypeIndexType index{};

	std::string name{};
	std::size_t size{};
};

constexpr RE_CORE_API TypeHashType HashStr(const std::string_view str)
{
	TypeHashType hash = 14695981039346656037ull;
	for (const char c : str)
	{
		hash ^= static_cast<TypeHashType>(c);
		hash *= 1099511628211ull;
	}

	return hash;
}

class RE_CORE_API TypeRegistry
{
public:
	static TypeRegistry& Get();

	TypeIndexType Register(TypeInfo* info);

private:
	TypeIndexType m_nextIndex = 0;

	mutable std::mutex m_mutex;

	std::vector<const TypeInfo*> m_indexToInfo;
	std::unordered_map<TypeHashType, TypeIndexType> m_hashToIndex;
};

template <typename T>
struct TypeFactory
{
	static const TypeInfo& Get();
};

class RE_CORE_API Type
{
public:
	explicit Type(const TypeInfo* info);

	template <typename T>
	static Type Of();

	[[nodiscard]] TypeIndexType Index() const;

	[[nodiscard]] TypeHashType Hash() const;

	[[nodiscard]] const char* Name() const;

	[[nodiscard]] TypeHashType Size() const;

private:
	const TypeInfo* m_info;
};

} // namespace re::meta

namespace re
{

template <typename T>
meta::Type TypeOf();

template <typename T>
const char* NameOf();

} // namespace re

#include <Core/Meta/TypeInfo.inl>