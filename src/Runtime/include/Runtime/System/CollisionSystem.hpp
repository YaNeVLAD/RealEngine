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
		SpatialGrid* grid = nullptr;
		for (auto&& [entity, gridComp] : *scene.CreateView<PhysicsGridComponent>())
		{
			grid = &gridComp.grid;
			break;
		}
		if (!grid)
		{
			return;
		}

		for (auto&& [entity, transform, collider, _] : *scene.CreateView<TransformComponent, ColliderComponent3D, Dirty<TransformComponent>>())
		{
			if (collider.m_isInserted)
			{
				grid->Remove(entity, collider.m_lastBounds);
			}

			AABB newBounds = collider.GetWorldBounds(transform);

			grid->Insert(entity, newBounds);

			collider.m_lastBounds = newBounds;
			collider.m_isInserted = true;
		}
	}
};

} // namespace re