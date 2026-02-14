#pragma once

#include <Core/Export.hpp>

#include <Core/HashedString.hpp>

#include <mutex>
#include <string>
#include <cstring>
#include <unordered_map>

namespace re::meta
{

using TypeIndexType = std::size_t;

struct TypeInfo
{
	Hash_t hash{};
	TypeIndexType index{};

	std::string name{};
	std::size_t size{};
};

class RE_CORE_API TypeRegistry
{
public:
	static TypeRegistry& Get();

	TypeIndexType Register(TypeInfo* info);

private:
	TypeIndexType m_nextIndex = 0;

	mutable std::mutex m_mutex;

	std::vector<const TypeInfo*> m_indexToInfo;
	std::unordered_map<Hash_t, TypeIndexType> m_hashToIndex;
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

	[[nodiscard]] Hash_t Hash() const;

	[[nodiscard]] const char* Name() const;

	[[nodiscard]] std::size_t Size() const;

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