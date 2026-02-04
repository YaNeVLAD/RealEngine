#include <ECS/EntityManager/EntityManager.hpp>

#include <cassert>

namespace re::ecs
{

[[nodiscard]] Entity EntityManager::CreateEntity()
{
	std::size_t index;

	if (!m_availableIndices.empty())
	{
		index = m_availableIndices.front();
		m_availableIndices.pop();
	}
	else
	{
		index = m_nextEntityIndex++;
	}

	if (index >= m_generations.size())
	{
		m_generations.resize(index + 1, 0);
		m_signatures.resize(index + 1);
		m_entityLocations.resize(index + 1);
	}

	const Entity entity(index, m_generations[index]);

	m_signatures[entity.Index()].reset();

	m_activeEntities.push_back(entity);
	m_entityLocations[entity.Index()] = m_activeEntities.size() - 1;

	return entity;
}

void EntityManager::DestroyEntity(const Entity entity)
{
	if (!IsValid(entity))
	{
		return;
	}

	const std::size_t index = entity.Index();

	m_generations[index]++;

	const size_t indexOfRemoved = m_entityLocations[index];
	const Entity lastEntity = m_activeEntities.back();

	m_activeEntities[indexOfRemoved] = lastEntity;
	m_entityLocations[lastEntity.Index()] = indexOfRemoved;
	m_activeEntities.pop_back();

	m_signatures[index].reset();
	m_availableIndices.push(index);
}

bool EntityManager::IsValid(const Entity entity) const
{
	const auto index = entity.Index();
	return index < m_generations.size() && entity.Generation() == m_generations[index];
}

void EntityManager::SetSignature(const Entity entity, Signature const& signature)
{
	assert(IsValid(entity) && "Entity is not valid");
	m_signatures[entity.Index()] = signature;
}

Signature& EntityManager::GetSignature(const Entity entity)
{
	assert(IsValid(entity) && "Entity is not valid");
	return m_signatures[entity.Index()];
}

Signature const& EntityManager::GetSignature(const Entity entity) const
{
	return const_cast<EntityManager&>(*this).GetSignature(entity);
}

[[nodiscard]] std::vector<Entity> const& EntityManager::GetActiveEntities() const
{
	return m_activeEntities;
}

} // namespace re::ecs