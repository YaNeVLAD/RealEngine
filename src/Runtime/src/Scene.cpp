#include <Runtime/Scene.hpp>

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
}

Entity Scene::CreateEntity()
{
	return Entity{ m_registry };
}

} // namespace re::runtime