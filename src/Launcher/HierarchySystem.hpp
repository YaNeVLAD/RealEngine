#pragma once

#include <ECS/Scene.hpp>
#include <Runtime/Components.hpp>

#include "Components.hpp"

class HierarchySystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, re::core::TimeDelta) override
	{
		for (auto&& [_, child, childTransform] : *scene.CreateView<ChildComponent, re::TransformComponent>())
		{
			if (const auto parent = child.parentEntity; scene.IsValid(parent))
			{
				const auto& [position, rotation, scale, _] = scene.GetComponent<re::TransformComponent>(parent);

				const auto [rotX, rotY] = child.localPosition.Rotate(rotation.z);

				childTransform.position = {
					position.x + rotX,
					position.y + rotY,
					position.z
				};

				childTransform.rotation = {
					rotation.x,
					rotation.y,
					rotation.z + child.localRotation
				};

				childTransform.scale = scale;
			}
		}
	}
};
