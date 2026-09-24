#include <ECS/Entity/Entity.hpp>

namespace re::ecs
{

constexpr Entity::Entity(const std::size_t id)
	: m_id(id)
{
}

constexpr Entity::Entity(const std::size_t index, const std::size_t generation)
	: m_id((generation << re::detail::ENTITY_INDEX_BITS) | index)
{
}

constexpr Entity::operator std::size_t() const
{
	return Index();
}

constexpr std::size_t Entity::Id() const
{
	return m_id;
}

constexpr std::size_t Entity::Index() const
{
	return m_id & re::detail::ENTITY_INDEX_MASK;
}

constexpr std::size_t Entity::Generation() const
{
	return (m_id >> re::detail::ENTITY_INDEX_BITS) & re::detail::ENTITY_GENERATION_MASK;
}

constexpr bool Entity::Valid() const
{
	return m_id != re::detail::ENTITY_INDEX_MASK;
}

inline constexpr Entity Entity::INVALID_ID{ re::detail::ENTITY_INDEX_MASK };

} // namespace re::ecs