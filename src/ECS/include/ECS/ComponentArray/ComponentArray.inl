#include <ECS/ComponentArray/IComponentArray.hpp>

#include <cassert>

namespace re::ecs
{

template <typename TComponent>
void ComponentArray<TComponent>::AddComponent(Entity entity, TComponent const& component)
{
	assert(!HasComponent(entity) && "Component already exists for this entity");

	const auto index = entity.Index();

	if (index >= m_sparse.size())
	{
		m_sparse.resize(index + 1, InvalidIndex);
	}

	const size_t denseIndex = m_components.size();
	m_sparse[index] = denseIndex;
	m_denseToEntity.push_back(entity);
	m_components.push_back(component);
}

template <typename TComponent>
void ComponentArray<TComponent>::RemoveComponent(Entity entity)
{
	if (!HasComponent(entity))
	{
		return;
	}

	const auto indexToRemove = entity.Index();
	const size_t denseIndexOfRemoved = m_sparse[indexToRemove];

	TComponent& lastComponent = m_components.back();
	Entity lastEntity = m_denseToEntity.back();

	if constexpr (std::movable<TComponent>)
	{
		m_components[denseIndexOfRemoved] = std::move(lastComponent);
	}
	else
	{
		m_components[denseIndexOfRemoved] = lastComponent;
	}
	m_denseToEntity[denseIndexOfRemoved] = lastEntity;

	m_sparse[lastEntity.Index()] = denseIndexOfRemoved;

	m_sparse[indexToRemove] = InvalidIndex;

	m_components.pop_back();
	m_denseToEntity.pop_back();
}

template <typename TComponent>
TComponent& ComponentArray<TComponent>::GetComponent(Entity entity)
{
	assert(HasComponent(entity) && "Entity does not have component of this type");
	return m_components[m_sparse[entity.Index()]];
}

template <typename TComponent>
TComponent const& ComponentArray<TComponent>::GetComponent(Entity entity) const
{
	return const_cast<ComponentArray&>(*this).GetComponent(entity);
}

template <typename TComponent>
bool ComponentArray<TComponent>::HasComponent(Entity entity) const
{
	const auto index = entity.Index();
	return index < m_sparse.size()
		&& m_sparse[index] != InvalidIndex
		&& m_denseToEntity[m_sparse[index]] == entity;
}

template <typename TComponent>
std::vector<TComponent>& ComponentArray<TComponent>::GetComponents()
{
	return m_components;
}

template <typename TComponent>
void ComponentArray<TComponent>::OnEntityDestroyed(Entity entity)
{
	if (HasComponent(entity))
	{
		RemoveComponent(entity);
	}
}

} // namespace re::ecs
