#pragma once

#include <Core/Meta/TypeInfo.hpp>

#include <typeinfo>

namespace re::ecs
{

using TypeIndexType = meta::TypeIndexType;

template <typename T>
TypeIndexType TypeIndex()
{
	static const TypeIndexType id = TypeOf<T>().Index();

	return id;
}

template <typename T>
const char* Name()
{
	return typeid(T).name();
}

} // namespace re::ecs