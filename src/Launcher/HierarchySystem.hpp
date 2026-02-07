#pragma once

#include <ECS/Scene/Scene.hpp>
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
				const auto& [position, rotation, scale] = scene.GetComponent<re::TransformComponent>(parent);

				const re::Vector2f rotatedOffset = child.localPosition.Rotate(rotation);

				childTransform.position = position + rotatedOffset;

				childTransform.rotation = rotation + child.localRotation;

				childTransform.scale = scale;
			}
		}
	}
};