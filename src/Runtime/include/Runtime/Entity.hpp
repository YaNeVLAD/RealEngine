#pragma once

#include <Runtime/Export.hpp>

#include <entt/entt.hpp>

namespace re::runtime
{

class RE_RUNTIME_API Entity
{
public:
	explicit Entity(entt::registry& registry);

	template <typename TComponent, typename... TArgs>
	Entity Add(TArgs&&... args);

	template <typename... TComponents>
	decltype(auto) Get() const;

private:
	entt::entity m_entity;
	entt::registry& m_registry;
};

} // namespace re::runtime

#include <Runtime/Entity.inl>