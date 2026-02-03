#pragma once

#include <ECS/EntityWrapper/EntityWrapper.hpp>

#include <vector>
#include <unordered_map>

namespace re::ecs
{

class Scene;

class System
{

public:
	using WrappedEntity = EntityWrapper<Scene>;

	virtual ~System() = default;

	virtual void Update(Scene& scene, float dt) = 0;

	std::vector<WrappedEntity> Entities;
	std::unordered_map<Entity, size_t> EntityToIndexMap;
};

} // namespace re::ecs