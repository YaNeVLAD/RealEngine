#include <ECS/ComponentManager/ComponentManager.hpp>

namespace re::ecs
{

template <typename TComponent>
void ComponentManager::RegisterComponent()
{
	const ComponentType componentType = TypeIndex<TComponent>();

	RE_ASSERT(!m_componentArrays.contains(componentType),
		"Can not register {} more than once",
		NameOf<TComponent>());

	m_componentArrays[componentType] = std::make_shared<ComponentArray<TComponent>>();
}

template <typename TComponent>
bool ComponentManager::IsComponentRegistered() const
{
	return m_componentArrays.contains(TypeIndex<TComponent>());
}

template <typename TComponent>
void ComponentManager::AddComponent(Entity entity, TComponent const& component)
{
	GetComponentArray<TComponent>()->AddComponent(entity, component);
}

template <typename TComponent>
void ComponentManager::RemoveComponent(Entity entity)
{
	GetComponentArray<TComponent>()->RemoveComponent(entity);
}

template <typename TComponent>
TComponent& ComponentManager::GetComponent(Entity entity)
{
	return GetComponentArray<TComponent>()->GetComponent(entity);
}

template <typename TComponent>
TComponent const& ComponentManager::GetComponent(Entity entity) const
{
	return GetComponentArray<TComponent>()->GetComponent(entity);
}

template <typename TComponent>
bool ComponentManager::HasComponent(Entity entity) const
{
	return GetComponentArray<TComponent>()->HasComponent(entity);
}

inline void ComponentManager::OnEntityDestroyed(Entity entity)
{
	for (auto const& array : m_componentArrays | std::views::values)
	{
		array->OnEntityDestroyed(entity);
	}
}

template <typename TComponent>
std::shared_ptr<ComponentArray<TComponent>> ComponentManager::GetComponentArray() const
{
	const ComponentType componentType = TypeIndex<TComponent>();

	RE_ASSERT(m_componentArrays.contains(componentType), "Component {} is not registered", NameOf<TComponent>());

	return std::static_pointer_cast<ComponentArray<TComponent>>(m_componentArrays.at(componentType));
}

} // namespace re::ecs