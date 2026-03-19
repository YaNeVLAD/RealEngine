#include <ECS/ComponentManager/ComponentManager.hpp>

namespace re::ecs
{

template <typename TComponent>
void ComponentManager::RegisterComponent()
{
	const ComponentType componentType = TypeIndex<TComponent>();

	if (componentType >= m_componentArrays.size())
	{
		m_componentArrays.resize(componentType + 1);
	}

	RE_ASSERT(m_componentArrays[componentType] == nullptr, "Component is already registered!");

	m_componentArrays[componentType] = std::make_shared<ComponentArray<TComponent>>();
}

template <typename TComponent>
bool ComponentManager::IsComponentRegistered() const
{
	const auto componentType = TypeIndex<TComponent>();
	if (componentType >= m_componentArrays.size())
	{
		return false;
	}

	return m_componentArrays[componentType] != nullptr;
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

inline void ComponentManager::OnEntityDestroyed(const Entity entity) const
{
	for (auto const& array : m_componentArrays)
	{
		if (array != nullptr)
		{
			array->OnEntityDestroyed(entity);
		}
	}
}

template <typename TComponent>
std::shared_ptr<ComponentArray<TComponent>> ComponentManager::GetComponentArray() const
{
	const ComponentType componentType = TypeIndex<TComponent>();

	RE_ASSERT(componentType < m_componentArrays.size() && m_componentArrays[componentType] != nullptr,
		"Component is not registered!");

	return std::static_pointer_cast<ComponentArray<TComponent>>(m_componentArrays[componentType]);
}

} // namespace re::ecs
