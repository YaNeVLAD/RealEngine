#pragma once

#include "Components.hpp"
#include "Constants.hpp"
#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

namespace arcanoid
{

struct BottomDeathZoneTag
{
};

class ArcanoidGameSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		UpdatePaddle(scene, dt);
		HandleCollisions(scene);
		UpdateBallAttached(scene);
		KeepBallSpeedConstant(scene);
	}

private:
	static void KeepBallSpeedConstant(re::ecs::Scene& scene)
	{
		for (auto&& [entity, ball, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
		{
			if (ball.isAttachedToPaddle)
			{
				continue;
			}

			const re::Vector3f currentVel = rb.linearVelocity;
			const float currentSpeed = std::sqrt(currentVel.x * currentVel.x + currentVel.z * currentVel.z);
			if (currentSpeed > std::numeric_limits<float>::epsilon())
			{
				// Если скорость отличается от желаемой (ball.speed)
				if (std::abs(currentSpeed - ball.speed) > 0.01f)
				{
					rb.linearVelocity.x = (currentVel.x / currentSpeed) * ball.speed;
					rb.linearVelocity.z = (currentVel.z / currentSpeed) * ball.speed;
					rb.isVelocityDirty = true;
				}
			}
			else
			{
				rb.linearVelocity = { 0.0f, 0.0f, -ball.speed };
				rb.isVelocityDirty = true;
			}
		}
	}

	static void UpdatePaddle(re::ecs::Scene& scene, const float dt)
	{
		for (auto&& [entity, transform, paddle, rb] : *scene.CreateView<re::TransformComponent, PaddleComponent, re::physics::RigidBody>())
		{
			rb.linearVelocity.x = paddle.moveDir * paddle.speed;
			rb.isVelocityDirty = true;

			const float limit = constants::FieldLimitX - (paddle.width * 0.5f);
			if (transform.position.x > limit || transform.position.x < -limit)
			{
				transform.position.x = std::clamp(transform.position.x, -limit, limit);
				rb.position = transform.position;
				scene.MakeDirty<re::TransformComponent>(entity);
			}
		}
	}

	static void UpdateBallAttached(re::ecs::Scene& scene)
	{
		re::Vector3f paddlePos;
		bool paddleFound = false;

		for (auto&& [entity, transform, paddle] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
		{
			paddlePos = transform.position;
			paddleFound = true;
			break;
		}

		for (auto&& [entity, transform, ball, rb] : *scene.CreateView<re::TransformComponent, BallComponent, re::physics::RigidBody>())
		{
			if (ball.isAttachedToPaddle && paddleFound)
			{
				transform.position.x = paddlePos.x;
				transform.position.z = paddlePos.z - 1.0f;

				rb.linearVelocity = { 0.f, 0.f, 0.f };
				rb.isVelocityDirty = true;

				scene.MakeDirty<re::TransformComponent>(entity);
			}
		}
	}

	static void HandleCollisions(re::ecs::Scene& scene)
	{
		const auto eventView = scene.CreateView<re::PhysicsEventsComponent>();
		if (eventView->begin() == eventView->end())
		{
			return;
		}

		auto& [collisions] = scene.GetComponent<re::PhysicsEventsComponent>(std::get<0>(*eventView->begin()));

		for (const auto& collision : collisions)
		{
			const re::ecs::Entity entityA{ collision.entityA };
			const re::ecs::Entity entityB{ collision.entityB };

			// Проверяем, жив ли еще объект (чтобы не обрабатывать одну пулю дважды)
			if (!scene.IsValid(entityA) || !scene.IsValid(entityB))
			{
				continue;
			}

			CheckBallCollision(scene, entityA, entityB);
			CheckBallCollision(scene, entityB, entityA); // Проверяем в обе стороны
		}
	}

	static void CheckBallCollision(re::ecs::Scene& scene, re::ecs::Entity ballEntity, re::ecs::Entity otherEntity)
	{
		if (!scene.HasComponent<BallComponent>(ballEntity))
		{
			return;
		}

		auto& ball = scene.GetComponent<BallComponent>(ballEntity);
		auto& ballRb = scene.GetComponent<re::physics::RigidBody>(ballEntity);
		auto& ballTrans = scene.GetComponent<re::TransformComponent>(ballEntity);

		// 1. Мяч + Кирпич
		if (scene.HasComponent<BrickComponent>(otherEntity))
		{
			auto& brick = scene.GetComponent<BrickComponent>(otherEntity);
			brick.health--;
			if (brick.health <= 0)
			{
				scene.DestroyEntity(otherEntity); // EnTT сам удалит Jolt Body!
				AddScore(scene, brick.scoreValue);
			}
		}
		// 2. Мяч + Ракетка (Особый аркадный отскок)
		else if (scene.HasComponent<PaddleComponent>(otherEntity))
		{
			auto& paddle = scene.GetComponent<PaddleComponent>(otherEntity);
			auto& paddleTrans = scene.GetComponent<re::TransformComponent>(otherEntity);

			// Jolt уже отбил мяч по оси Z, мы только корректируем ось X в зависимости от места удара
			const float hitFactor = (ballTrans.position.x - paddleTrans.position.x) / (paddle.width * 0.5f);

			ballRb.linearVelocity.z = -std::abs(ballRb.linearVelocity.z); // Гарантируем отскок вверх
			ballRb.linearVelocity.x = hitFactor * ball.speed * constants::PaddleBounceFactor;

			NormalizeSpeed(ballRb.linearVelocity, ball.speed);
			ballRb.isVelocityDirty = true;
		}
		// 3. Мяч + Зона Смерти
		else if (scene.HasComponent<BottomDeathZoneTag>(otherEntity))
		{
			LoseLife(scene);
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