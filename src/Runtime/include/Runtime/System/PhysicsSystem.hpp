#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <Physics/IPhysicsWorld.hpp>
#include <Runtime/Components.hpp>

#include <memory>

namespace re
{

class RE_RUNTIME_API PhysicsSystem final : public ecs::System
{
public:
	explicit PhysicsSystem(ecs::Scene& scene);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity) const;

private:
	std::unique_ptr<physics::IPhysicsWorld> m_world;
};

} // namespace re