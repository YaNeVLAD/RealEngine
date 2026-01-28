#include <Runtime/Entity.hpp>

namespace re::runtime
{

Entity::Entity(entt::registry& registry)
	: m_entity(registry.create())
	, m_registry(registry)
{
}

} // namespace re::runtime