#include <ECS/Entity/Entity.hpp>

namespace
{

constexpr std::size_t ENTITY_INDEX_BITS = 32;
constexpr std::size_t ENTITY_GENERATION_BITS = 32;

constexpr std::size_t ENTITY_INDEX_MASK = (1ULL << ENTITY_INDEX_BITS) - 1;
constexpr std::size_t ENTITY_GENERATION_MASK = (1ULL << ENTITY_GENERATION_BITS) - 1;

} // namespace

namespace re::ecs
{

const auto Entity::INVALID_ID = Entity{ ENTITY_INDEX_MASK };

Entity::Entity(const std::size_t id)
	: m_id(id)
{
}

Entity::Entity(const std::size_t index, const std::size_t generation)
	: m_id((generation << ENTITY_INDEX_BITS) | index)
{
}

Entity::operator std::size_t() const
{
	return Index();
}

std::size_t Entity::Id() const
{
	return m_id;
}

std::size_t Entity::Index() const
{
	return m_id & ENTITY_INDEX_MASK;
}

std::size_t Entity::Generation() const
{
	return (m_id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

} // namespace re::ecs