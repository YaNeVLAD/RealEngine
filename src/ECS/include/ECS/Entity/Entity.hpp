#pragma once

#include <ECS/Export.hpp>

#include <compare>
#include <cstddef>

namespace re::detail
{

inline constexpr std::size_t ENTITY_INDEX_BITS = 32;
inline constexpr std::size_t ENTITY_GENERATION_BITS = 32;

inline constexpr std::size_t ENTITY_INDEX_MASK = (1ULL << ENTITY_INDEX_BITS) - 1;
inline constexpr std::size_t ENTITY_GENERATION_MASK = (1ULL << ENTITY_GENERATION_BITS) - 1;

} // namespace re::detail

namespace re::ecs
{

class Entity final
{
public:
	constexpr explicit Entity(std::size_t id);

	constexpr Entity(std::size_t index, std::size_t generation);

	constexpr operator std::size_t() const;

	static const Entity INVALID_ID;

	[[nodiscard]] constexpr std::size_t Id() const;

	[[nodiscard]] constexpr std::size_t Index() const;

	[[nodiscard]] constexpr std::size_t Generation() const;

	[[nodiscard]] constexpr bool Valid() const;

	constexpr auto operator<=>(Entity const&) const = default;

private:
	std::size_t m_id{};
};

} // namespace re::ecs

template <>
struct std::hash<re::ecs::Entity>
{
	std::size_t operator()(re::ecs::Entity const& entity) const noexcept
	{
		return entity.Id();
	}
};

#include <ECS/Entity/Entity.inl>