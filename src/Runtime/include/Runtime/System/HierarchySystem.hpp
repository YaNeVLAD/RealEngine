#pragma once

#include <Runtime/Export.hpp>

#include <ECS/Scene.hpp>
#include <Runtime/Components.hpp>

#include <glm/gtx/euler_angles.hpp>

namespace re
{

struct HierarchyComponent
{
	ecs::Entity parent = ecs::Entity::INVALID_ID;

	Vector3f localPosition = Vector3f(0.f);
	Vector3f localRotation = Vector3f(0.f);
	Vector3f localScale = Vector3f(1.f);
};

class RE_RUNTIME_API HierarchySystem final : public ecs::System
{
public:
	void Update(ecs::Scene& scene, core::TimeDelta) override
	{
		std::vector<ecs::Entity> orphansToDestroy;

		for (auto&& [entity, hierarchy, transform] : *scene.CreateView<HierarchyComponent, TransformComponent>())
		{
			if (scene.IsValid(hierarchy.parent) && scene.HasComponent<TransformComponent>(hierarchy.parent))
			{
				const auto& parentTransform = scene.GetComponent<TransformComponent>(hierarchy.parent);

				glm::mat4 parentMat = glm::translate(glm::mat4(1.0f), glm::vec3(parentTransform.position.x, parentTransform.position.y, parentTransform.position.z));

				parentMat *= glm::eulerAngleYXZ(
					glm::radians(parentTransform.rotation.y),
					glm::radians(parentTransform.rotation.x),
					glm::radians(parentTransform.rotation.z));

				parentMat = glm::scale(parentMat, glm::vec3(parentTransform.scale.x, parentTransform.scale.y, parentTransform.scale.z));

				glm::vec4 worldPos = parentMat * glm::vec4(hierarchy.localPosition.x, hierarchy.localPosition.y, hierarchy.localPosition.z, 1.0f);

				transform.position = { worldPos.x, worldPos.y, worldPos.z };

				transform.rotation = parentTransform.rotation + hierarchy.localPosition;

				transform.scale = parentTransform.scale * hierarchy.localScale;

				scene.MakeDirty<TransformComponent>(entity);
			}
			else
			{
				orphansToDestroy.push_back(entity);
			}
		}

		for (const auto orphan : orphansToDestroy)
		{
			scene.DestroyEntity(orphan);
		}
	}
};

} // namespace re