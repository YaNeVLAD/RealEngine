#pragma once

#include <compare>
#include <cstddef>

namespace re::ecs::details
{

constexpr std::size_t ENTITY_INDEX_BITS = 32;
constexpr std::size_t ENTITY_GENERATION_BITS = 32;

constexpr std::size_t ENTITY_INDEX_MASK = (1ULL << ENTITY_INDEX_BITS) - 1;
constexpr std::size_t ENTITY_GENERATION_MASK = (1ULL << ENTITY_GENERATION_BITS) - 1;

} // namespace re::ecs::details

namespace re::ecs
{

struct Entity
{
	std::size_t id;

	operator std::size_t()
	{
		return Index();
	}

	operator std::size_t() const
	{
		return Index();
	}

	[[nodiscard]] std::size_t Index() const
	{
		return id & details::ENTITY_INDEX_MASK;
	}

	[[nodiscard]] std::size_t Generation() const
	{
		return (id >> details::ENTITY_INDEX_BITS) & details::ENTITY_GENERATION_MASK;
	}

	auto operator<=>(Entity const&) const = default;
};

[[nodiscard]] constexpr Entity CreateEntity(const std::size_t index, const std::size_t generation)
{
	return Entity{ (generation << details::ENTITY_INDEX_BITS) | index };
}

constexpr auto InvalidEntity = Entity{ details::ENTITY_INDEX_MASK };

} // namespace re::ecs

template <>
struct std::hash<re::ecs::Entity>
{
	std::size_t operator()(re::ecs::Entity const& entity) const noexcept
	{
		return entity.id;
	}
}; // namespace std