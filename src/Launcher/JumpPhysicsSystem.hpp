#pragma once

#include <ECS/Scene/Scene.hpp>
#include <Runtime/Components.hpp>

#include "Components.hpp"

class JumpPhysicsSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		constexpr float GRAVITY = 2000.0f;
		constexpr float JUMP_FORCE = -900.0f;

		for (auto&& [_, transform, physics] : *scene.CreateView<re::TransformComponent, GravityComponent>())
		{
			if (physics.startY == 0.0f)
			{
				physics.startY = transform.position.y;
			}

			if (physics.phaseTime > 0.0f)
			{
				physics.phaseTime -= dt;
				return;
			}

			physics.velocity.y += GRAVITY * dt;
			transform.position.y += physics.velocity.y * dt;

			if (transform.position.y >= physics.startY)
			{
				transform.position.y = physics.startY;
				physics.velocity.y = JUMP_FORCE;
			}
		}
	}
};