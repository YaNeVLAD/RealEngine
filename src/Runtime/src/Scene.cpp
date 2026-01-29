#include <Runtime/Scene.hpp>

#include <Runtime/Entity.hpp>
#include <Runtime/System.hpp>

namespace re::runtime
{

Scene::Scene()
{
}

Scene::~Scene()
{
	Clear();
}

void Scene::Clear()
{
	m_registry.clear();
	m_systems.clear();
}

Entity Scene::CreateEntity()
{
	return Entity{ m_registry.create(), m_registry };
}

void Scene::DestroyEntity(const Entity entity)
{
	m_registry.destroy(static_cast<entt::entity>(entity));
}

void Scene::OnUpdate(const core::TimeDelta dt)
{
	for (const auto& system : m_systems)
	{
		system->OnUpdate(*this, dt);
	}
}

} // namespace re::runtime