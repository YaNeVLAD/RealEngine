#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Physics/Types.hpp>
#include <Runtime/Components.hpp>

struct MazePlayerStateComponent
{
	static constexpr float MOVE_SPEED = 6.0f;

	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
};

struct MazeWallComponent
{
	float x, z;
	float halfSize;
};

class MazePlayerControllerSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta) override
	{
		for (auto&& [entity, transform, player, rb] : *scene.CreateView<re::TransformComponent, MazePlayerStateComponent, re::RigidBodyComponent>())
		{
			glm::vec3 front;
			front.x = std::cos(glm::radians(transform.rotation.y)) * std::cos(glm::radians(transform.rotation.x));
			front.y = 0.f;
			front.z = std::sin(glm::radians(transform.rotation.y)) * std::cos(glm::radians(transform.rotation.x));
			front = glm::normalize(front);

			const glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.f, 1.f, 0.f)));

			glm::vec3 moveDir(0.0f);
			if (player.forward)
			{
				moveDir += front;
			}
			if (player.backward)
			{
				moveDir -= front;
			}
			if (player.left)
			{
				moveDir -= right;
			}
			if (player.right)
			{
				moveDir += right;
			}

			if (glm::length(moveDir) > std::numeric_limits<float>::epsilon())
			{
				moveDir = glm::normalize(moveDir) * player.MOVE_SPEED;
			}

			rb.linearVelocity.x = moveDir.x;
			rb.linearVelocity.z = moveDir.z;

			rb.isVelocityDirty = true;
		}
	}
};