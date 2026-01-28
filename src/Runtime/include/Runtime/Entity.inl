#include <Runtime/Entity.hpp>

namespace re::runtime
{

template <typename TComponent, typename... TArgs>
Entity Entity::Add(TArgs&&... args)
{
	m_registry.emplace<TComponent>(m_entity, std::forward<TArgs>(args)...);

	return *this;
}

template <typename... TComponents>
decltype(auto) Entity::Get() const
{
	return m_registry.get<TComponents...>(m_entity);
}

} // namespace re::runtime
