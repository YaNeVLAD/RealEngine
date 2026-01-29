#include <Runtime/Entity.hpp>

namespace re::runtime
{

inline Entity::Entity(const entt::entity entity, entt::registry& registry)
	: m_entity(entity)
	, m_registry(registry)
{
}

inline void Entity::Invalidate()
{
	m_entity = entt::null;
}

inline bool Entity::IsValid() const
{
	return m_entity != entt::null;
}

inline Entity::operator bool() const
{
	return IsValid();
}

inline Entity::operator entt::entity() const
{
	return m_entity;
}

inline void Entity::AssertValid() const
{
	if (!IsValid())
	{
		throw std::invalid_argument("Invalid entity");
	}
}

template <typename TComponent, typename... TArgs>
Entity Entity::Add(TArgs&&... args)
{
	AssertValid();

	m_registry.emplace_or_replace<TComponent>(m_entity, std::forward<TArgs>(args)...);

	return *this;
}

template <typename... TComponents>
decltype(auto) Entity::Get()
{
	AssertValid();

	return m_registry.get<TComponents...>(m_entity);
}

template <typename... TComponents>
decltype(auto) Entity::Get() const
{
	AssertValid();

	return m_registry.get<TComponents...>(m_entity);
}

template <typename... TComponents>
bool Entity::Has() const
{
	AssertValid();

	return m_registry.all_of<TComponents...>(m_entity);
}

template <typename TComponent>
void Entity::Remove()
{
	AssertValid();

	m_registry.remove<TComponent>(m_entity);
}

inline bool Entity::operator==(const Entity& other) const
{
	return m_entity == other.m_entity;
}

inline bool Entity::operator!=(const Entity& other) const
{
	return !(*this == other);
}

} // namespace re::runtime
