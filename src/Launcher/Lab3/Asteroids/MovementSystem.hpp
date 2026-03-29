#pragma once

#include <Core/Types.hpp>
#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include "AsteroidsComponents.hpp"

class MovementSystem final : public re::ecs::System
{
public:
	explicit MovementSystem(re::render::IWindow& window)
		: m_window(window)
	{
	}

	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		for (auto&& [entity, transform, vel] : *scene.CreateView<re::TransformComponent, VelocityComponent>())
		{
			transform.position.x += vel.linear.x * dt;
			transform.position.y += vel.linear.y * dt;
			transform.rotation.z += vel.angular * dt;
		}

		const auto [halfWidth, halfHeight] = m_window.Size() / 2u;
		const auto halfWidthF = static_cast<float>(halfWidth);
		const auto halfHeightF = static_cast<float>(halfHeight);
		for (auto&& [entity, transform, wrap] : *scene.CreateView<re::TransformComponent, ScreenWrapComponent>())
		{
			const float r = wrap.radius;
			if (transform.position.x > halfWidthF + r)
			{
				transform.position.x = -halfWidthF - r;
			}
			else if (transform.position.x < -halfWidthF - r)
			{
				transform.position.x = halfWidthF + r;
			}

			if (transform.position.y > halfHeightF + r)
			{
				transform.position.y = -halfHeightF - r;
			}
			else if (transform.position.y < -halfHeightF - r)
			{
				transform.position.y = halfHeightF + r;
			}
		}
	}

private:
	re::render::IWindow& m_window;
};