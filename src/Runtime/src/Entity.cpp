#include <Runtime/Entity.hpp>

namespace re::runtime
{

Entity::Entity(entt::registry& registry)
	: m_entity(registry.create())
	, m_registry(registry)
{
	m_registry.emplace<int>(m_entity, 42);
}

} // namespace re::runtime