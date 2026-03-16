#pragma once

#include <Core/Types.hpp>
#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include "AsteroidsComponents.hpp"

class MovementSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		for (auto&& [entity, transform, vel] : *scene.CreateView<re::TransformComponent, VelocityComponent>())
		{
			transform.position.x += vel.linear.x * dt;
			transform.position.y += vel.linear.y * dt;
			transform.rotation.z += vel.angular * dt;
		}

		for (auto&& [entity, transform, wrap] : *scene.CreateView<re::TransformComponent, ScreenWrapComponent>())
		{
			const float r = wrap.radius;
			if (transform.position.x > 960.f + r)
			{
				transform.position.x = -960.f - r;
			}
			else if (transform.position.x < -960.f - r)
			{
				transform.position.x = 960.f + r;
			}

			if (transform.position.y > 540.f + r)
			{
				transform.position.y = -540.f - r;
			}
			else if (transform.position.y < -540.f - r)
			{
				transform.position.y = 540.f + r;
			}
		}
	}
};