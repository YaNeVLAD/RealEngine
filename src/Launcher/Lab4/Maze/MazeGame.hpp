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
		re::SpatialGrid* grid = nullptr;
		for (auto&& [entity, gridComp] : *scene.CreateView<re::PhysicsGridComponent>())
		{
			grid = &gridComp.grid;
			break;
		}

		if (!grid)
		{
			return;
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

			if (glm::length(moveDir) > std::numeric_limits<float>::epsilon())
			{
				moveDir = glm::normalize(moveDir) * player.MOVE_SPEED * dt;

				re::Vector3f newPos = transform.position;

				newPos.x += moveDir.x;
				if (CheckCollision(scene, *grid, newPos))
				{
					newPos.x -= moveDir.x;
				}

				newPos.z += moveDir.z;
				if (CheckCollision(scene, *grid, newPos))
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

	static bool CheckCollision(re::ecs::Scene& scene, const re::SpatialGrid& grid, const re::Vector3f& pos)
	{
		const re::AABB playerBounds = re::AABB::FromCenterSize(pos, PLAYER_RADIUS);

		std::vector<re::ecs::Entity> potentialColliders = grid.Query(playerBounds);
		return std::ranges::any_of(potentialColliders, [&](const re::ecs::Entity wallEntity) {
			if (!scene.HasComponent<MazeWallComponent>(wallEntity))
			{
				return false;
			}

			const auto& [x, z, halfSize] = scene.GetComponent<MazeWallComponent>(wallEntity);

			const float closestX = std::clamp(pos.x, x - halfSize, x + halfSize);
			const float closestZ = std::clamp(pos.z, z - halfSize, z + halfSize);

			const float dx = pos.x - closestX;
			const float dz = pos.z - closestZ;

			return ((dx * dx + dz * dz) < (PLAYER_RADIUS * PLAYER_RADIUS));
		});
	}
};