#pragma once

#include <ECS/Export.hpp>

#include <ECS/Entity/Entity.hpp>
#include <ECS/Entity/Signature.hpp>
#include <ECS/TypeIndex/TypeIndex.hpp>

#include <cassert>
#include <queue>
#include <vector>

namespace re::ecs
{

class RE_ECS_API EntityManager final
{
public:
	[[nodiscard]] Entity CreateEntity();

	void DestroyEntity(Entity entity);

	void SetSignature(Entity entity, Signature const& signature);

	Signature& GetSignature(Entity entity);

	[[nodiscard]] Signature const& GetSignature(Entity entity) const;

	[[nodiscard]] std::vector<Entity> const& GetActiveEntities() const;

	[[nodiscard]] bool IsValid(Entity entity) const;

private:
	TypeIndexType m_nextEntityIndex = 0;
	std::queue<TypeIndexType> m_availableIndices;

	std::vector<TypeIndexType> m_generations;

	std::vector<Signature> m_signatures;

	std::vector<Entity> m_activeEntities;
	std::vector<size_t> m_entityLocations;
};

} // namespace re::ecs