#pragma once

#include <Runtime/Export.hpp>

#include <Runtime/Entity.hpp>

#include <entt/entt.hpp>

namespace re::runtime
{

class RE_RUNTIME_API Scene
{
public:
	Scene();
	~Scene();

	void Clear();

	Entity CreateEntity();

private:
	entt::registry m_registry{};
};

} // namespace re::runtime