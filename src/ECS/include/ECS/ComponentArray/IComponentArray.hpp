#pragma once

#include <ECS/Entity/Entity.hpp>

namespace re::ecs
{

class IComponentArray
{
public:
	virtual ~IComponentArray() = default;

	virtual void OnEntityDestroyed(Entity entity) = 0;
};

} // namespace Engine::ecs
