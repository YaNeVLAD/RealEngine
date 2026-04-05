#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Physics/SpatialGrid.hpp>

#include "Components.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace arcanoid
{

class ArcanoidGameSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		auto* gridComponent = scene.FindComponent<re::PhysicsGridComponent>();
		if (!gridComponent)
		{
			return;
		}
		const auto& grid = gridComponent->grid;

		UpdatePaddle(scene, dt);
		UpdateBall(scene, grid, dt);
	}

private:
	static void UpdatePaddle(re::ecs::Scene& scene, const float dt)
	{
		for (auto&& [entity, transform, paddle] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
		{
			if (paddle.moveDir != 0)
			{
				transform.position.x += paddle.moveDir * paddle.speed * dt;

				const float limit = constants::FieldLimitX - (paddle.width * 0.5f);
				transform.position.x = std::clamp(transform.position.x, -limit, limit);

				scene.MakeDirty<re::TransformComponent>(entity);
			}
		}
	}

	static void UpdateBall(re::ecs::Scene& scene, const re::SpatialGrid& grid, const float dt)
	{
		re::Vector3f paddlePos;
		bool paddleFound = false;

		for (auto&& [entity, transform, paddle] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
		{
			paddlePos = transform.position;
			paddleFound = true;
			break;
		}

		for (auto&& [entity, transform, ball] : *scene.CreateView<re::TransformComponent, BallComponent>())
		{
			if (ball.isAttachedToPaddle && paddleFound)
			{
				transform.position.x = paddlePos.x;
				transform.position.z = paddlePos.z - 1.0f;
				scene.MakeDirty<re::TransformComponent>(entity);
				continue;
			}

			re::Vector3f nextPos = transform.position;
			nextPos.x += ball.velocity.x * dt;
			nextPos.z += ball.velocity.z * dt;

			if (nextPos.x > constants::FieldLimitX || nextPos.x < -constants::FieldLimitX)
			{
				ball.velocity.x = -ball.velocity.x;
				nextPos.x = transform.position.x;
			}
			if (nextPos.z < constants::FieldTopZ)
			{
				ball.velocity.z = -ball.velocity.z;
				nextPos.z = transform.position.z;
			}
			if (nextPos.z > constants::FieldBottomZ)
			{
				LoseLife(scene);
				return;
			}

			re::AABB ballBounds = re::AABB::FromCenterSize(nextPos, ball.radius * 2.0f);
			ballBounds = ballBounds.Swept(ball.velocity * dt);

			std::vector<re::ecs::Entity> potentialColliders = grid.Query(ballBounds);
			bool collisionHandled = false;

			for (const re::ecs::Entity colliderEntity : potentialColliders)
			{
				if (!scene.HasComponent<re::ColliderComponent3D>(colliderEntity) || !scene.HasComponent<re::TransformComponent>(colliderEntity))
				{
					continue;
				}

				const auto& colliderTrans = scene.GetComponent<re::TransformComponent>(colliderEntity);
				const auto& colliderComp = scene.GetComponent<re::ColliderComponent3D>(colliderEntity);

				if (re::AABB colliderBounds = colliderComp.GetWorldBounds(colliderTrans); ballBounds.Intersects(colliderBounds))
				{
					if (scene.HasComponent<PaddleComponent>(colliderEntity))
					{
						const auto& paddle = scene.GetComponent<PaddleComponent>(colliderEntity);

						ball.velocity.z = -std::abs(ball.velocity.z);

						const float hitFactor = (nextPos.x - colliderBounds.Center().x) / (paddle.width * 0.5f);
						ball.velocity.x = hitFactor * ball.speed * constants::PaddleBounceFactor;

						NormalizeSpeed(ball.velocity, ball.speed);

						nextPos.z = colliderTrans.position.z - 1.0f;

						collisionHandled = true;
						break;
					}
					if (scene.HasComponent<BrickComponent>(colliderEntity))
					{
						auto& brick = scene.GetComponent<BrickComponent>(colliderEntity);

						const float dx = nextPos.x - colliderBounds.Center().x;
						const float dz = nextPos.z - colliderBounds.Center().z;

						if (std::abs(dx) > std::abs(dz))
						{
							ball.velocity.x = -ball.velocity.x;
						}
						else
						{
							ball.velocity.z = -ball.velocity.z;
						}

						brick.health--;
						if (brick.health <= 0)
						{
							scene.DestroyEntity(colliderEntity);
							AddScore(scene, brick.scoreValue);
						}
						collisionHandled = true;
						break;
					}
				}
			}

			if (!collisionHandled)
			{
				transform.position = nextPos;
			}

			scene.MakeDirty<re::TransformComponent>(entity);
		}
	}

	static void NormalizeSpeed(re::Vector3f& vel, float targetSpeed)
	{
		const float currentSpeed = std::sqrt(vel.x * vel.x + vel.z * vel.z);
		if (currentSpeed > std::numeric_limits<float>::epsilon())
		{
			vel.x = (vel.x / currentSpeed) * targetSpeed;
			vel.z = (vel.z / currentSpeed) * targetSpeed;
		}
	}

	static void LoseLife(re::ecs::Scene& scene)
	{
		for (auto&& [entity, state] : *scene.CreateView<GameStateComponent>())
		{
			state.lives--;
			if (state.lives <= 0)
			{
				state.lives = constants::StartingLives;
				state.score = 0;
				state.currentLevel = 1;
				state.requestNextLevel = true;
			}
			break;
		}

		for (auto&& [entity, ball] : *scene.CreateView<BallComponent>())
		{
			ball.isAttachedToPaddle = true;
			ball.velocity = { 0.f, 0.f, 0.f };
		}
	}

	static void AddScore(re::ecs::Scene& scene, const int score)
	{
		for (auto&& [entity, state] : *scene.CreateView<GameStateComponent>())
		{
			state.score += score;
			break;
		}
	}
};

} // namespace arcanoid