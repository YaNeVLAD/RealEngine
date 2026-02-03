#pragma once

#include <Core/Types.hpp>
#include <ECS/EntityWrapper/EntityWrapper.hpp>

#include <unordered_map>
#include <vector>

namespace re::ecs
{

class Scene;

class System
{

public:
	using WrappedEntity = EntityWrapper<Scene>;

	virtual ~System() = default;

	virtual void Update(Scene& scene, core::TimeDelta dt) = 0;

	std::vector<WrappedEntity> Entities;
	std::unordered_map<Entity, size_t> EntityToIndexMap;
};

} // namespace re::ecs