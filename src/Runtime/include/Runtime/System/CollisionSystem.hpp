#pragma once

#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Physics/SpatialGrid.hpp>

namespace re
{

class CollisionSystem final : public ecs::System
{
public:
	void Update(ecs::Scene& scene, const core::TimeDelta) override
	{
		auto* gridComponent = scene.FindComponent<PhysicsGridComponent>();
		if (!gridComponent)
		{
			return;
		}
		auto& grid = gridComponent->grid;

		for (auto&& [entity, transform, collider, _] : *scene.CreateView<TransformComponent, ColliderComponent3D, Dirty<TransformComponent>>())
		{
			if (collider.m_isInserted)
			{
				grid.Remove(entity, collider.m_lastBounds);
			}

			AABB newBounds = collider.GetWorldBounds(transform);

			grid.Insert(entity, newBounds);

			collider.m_lastBounds = newBounds;
			collider.m_isInserted = true;
		}
	}
};

} // namespace re