#pragma once

#include <ECS/Export.hpp>

#include <compare>
#include <cstddef>

namespace re::ecs
{

class RE_ECS_API Entity final
{
public:
	explicit Entity(std::size_t id);

	Entity(std::size_t index, std::size_t generation);

	operator std::size_t() const;

	static const Entity INVALID_ID;

	[[nodiscard]] std::size_t Id() const;

	[[nodiscard]] std::size_t Index() const;

	[[nodiscard]] std::size_t Generation() const;

	auto operator<=>(Entity const&) const = default;

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