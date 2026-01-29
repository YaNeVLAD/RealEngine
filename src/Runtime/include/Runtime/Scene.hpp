#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>

#include <memory>
#include <vector>

#include <entt/entt.hpp>

namespace re::runtime
{

class Entity;
class System;

class RE_RUNTIME_API Scene
{
public:
	Scene();
	~Scene();

	Entity CreateEntity();

	void DestroyEntity(Entity entity);

	void OnUpdate(core::TimeDelta dt);

	template <typename TSystem, typename... TArgs>
	TSystem& AddSystem(TArgs&&... args);

	template <typename... TComponents>
	decltype(auto) View();

	void Clear();

private:
	entt::registry m_registry{};
	std::vector<std::unique_ptr<System>> m_systems{};
};

} // namespace re::runtime

#include <Runtime/Scene.inl>