#pragma once

#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <vector>

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
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		std::vector<MazeWallComponent> walls;
		for (auto&& [entity, wall] : *scene.CreateView<MazeWallComponent>())
		{
			walls.push_back(wall);
		}

		for (auto&& [entity, transform, player] : *scene.CreateView<re::TransformComponent, MazePlayerStateComponent>())
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

			if (glm::length(moveDir) > 0.001f)
			{
				moveDir = glm::normalize(moveDir) * player.MOVE_SPEED * dt;

				re::Vector3f newPos = transform.position;

				newPos.x += moveDir.x;
				if (CheckCollision(newPos, walls))
				{
					newPos.x -= moveDir.x;
				}

				newPos.z += moveDir.z;
				if (CheckCollision(newPos, walls))
				{
					newPos.z -= moveDir.z;
				}

				transform.position = newPos;
				scene.MakeDirty<re::TransformComponent>(entity);
			}
		}
	}

private:
	static constexpr float PLAYER_RADIUS = 0.4f;

	static bool CheckCollision(const re::Vector3f& pos, const std::vector<MazeWallComponent>& walls)
	{
		for (const auto& [x, z, halfSize] : walls)
		{
			const float closestX = std::clamp(pos.x, x - halfSize, x + halfSize);
			const float closestZ = std::clamp(pos.z, z - halfSize, z + halfSize);

			const float dx = pos.x - closestX;
			const float dz = pos.z - closestZ;

			if ((dx * dx + dz * dz) < (PLAYER_RADIUS * PLAYER_RADIUS))
			{ // distance(player, point) < PLAYER_RADIUS
				return true;
			}
		}
		return false;
	}
};