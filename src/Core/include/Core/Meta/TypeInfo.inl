#include <Core/Meta/TypeInfo.hpp>

namespace re::meta
{
template <typename T>
const TypeInfo& TypeFactory<T>::Get()
{
	static const TypeInfo info = [] {
		using ClearType = std::decay_t<T>;

		TypeInfo ti;
		ti.name = typeid(ClearType).name();
		ti.hash = HashStr(ti.name);
		ti.size = sizeof(ClearType);

		TypeRegistry::Get().Register(&ti);

		return ti;
	}();

	return info;
}

template <typename T>
Type Type::Of()
{
	return Type(&TypeFactory<T>::Get());
}

} // namespace re::meta

namespace re
{

template <typename T>
meta::Type TypeOf()
{
	return meta::Type::Of<T>();
}

template <typename T>
const char* NameOf()
{
	return meta::Type::Of<T>().Name();
}

} // namespace re