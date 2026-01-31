#pragma once

#include <ECS/Entity/Entity.hpp>
#include <ECS/Entity/Signature.hpp>

namespace re::ecs
{

class IView
{
public:
	virtual ~IView() = default;

	virtual void OnEntitySignatureChanged(Entity entity, Signature signature) = 0;
	virtual void OnEntityDestroyed(Entity entity) = 0;
};

} // namespace re::ecs