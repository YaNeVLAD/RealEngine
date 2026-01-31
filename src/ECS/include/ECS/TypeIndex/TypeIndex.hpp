#pragma once

#include <cstddef>

namespace re::ecs::details
{

class TypeIndexGenerator final
{
public:
	template <typename>
	static std::size_t Get()
	{
		static const std::size_t id = m_counter++;

		return id;
	}

private:
	inline static std::size_t m_counter = 0;
};

} // namespace re::ecs::details

namespace re::ecs
{

using TypeIndexType = std::size_t;

template <typename T>
TypeIndexType TypeIndex()
{
	return details::TypeIndexGenerator::Get<T>();
}

template <typename T>
const char* Name()
{
	return typeid(T).name();
}

} // namespace re::ecs