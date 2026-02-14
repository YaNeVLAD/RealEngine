#include <Core/Meta/TypeInfo.hpp>

#include <cassert>

namespace re::meta
{

TypeRegistry& TypeRegistry::Get()
{
	static TypeRegistry instance;

	return instance;
}

TypeIndexType TypeRegistry::Register(TypeInfo* info)
{
	std::lock_guard lock(m_mutex);

	if (const auto it = m_hashToIndex.find(info->hash); it != m_hashToIndex.end())
	{
		info->index = it->second;

		return it->second;
	}

	const auto index = m_nextIndex++;
	m_hashToIndex[info->hash] = index;

	m_indexToInfo.resize(index + 1);
	m_indexToInfo[index] = info;

	info->index = index;

	return index;
}

Type::Type(const TypeInfo* info)
	: m_info(info)
{
}

TypeIndexType Type::Index() const
{
	assert(m_info);

	return m_info->index;
}

Hash_t Type::Hash() const
{
	assert(m_info);

	return m_info->hash;
}

const char* Type::Name() const
{
	assert(m_info);

	return m_info->name.c_str();
}

std::size_t Type::Size() const
{
	assert(m_info);

	return m_info->size;
}

} // namespace re::meta