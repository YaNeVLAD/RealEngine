#pragma once

#include <Runtime/Export.hpp>

#include <entt/entt.hpp>

namespace re::runtime
{

class RE_RUNTIME_API Entity
{
public:
	explicit Entity(entt::registry& registry);

private:
	entt::entity m_entity;
	entt::registry& m_registry;
};

} // namespace re::runtime