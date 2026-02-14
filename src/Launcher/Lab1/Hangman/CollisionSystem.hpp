#pragma once

#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

class CollisionSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, re::core::TimeDelta dt) override
	{
		for (auto&& [_, transform, collider] : *scene.CreateView<re::TransformComponent, re::BoxColliderComponent>())
		{
			collider.position = transform.position;

			const re::Vector2f size = {
				collider.size.x * transform.scale.x,
				collider.size.y * transform.scale.y
			};

			collider.size = size;
		}
	}
};