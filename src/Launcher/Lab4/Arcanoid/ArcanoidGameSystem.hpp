#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include "Components.hpp"
#include "Constants.hpp"

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
		UpdatePaddle(scene);
		HandleCollisions(scene);
		UpdateBallAttached(scene);
		KeepBallSpeedConstant(scene);
		AnimateBallRotation(scene, dt);
	}

private:
	static void AnimateBallRotation(re::ecs::Scene& scene, const float dt)
	{
		for (auto&& [entity, transform, ball, rb] : *scene.CreateView<re::TransformComponent, BallComponent, re::physics::RigidBody>())
		{
			if (ball.isAttachedToPaddle)
			{
				continue;
			}

			const float spinFactor = constants::BallRotation * dt;

			transform.rotation.x += rb.linearVelocity.z * spinFactor;
			transform.rotation.z -= rb.linearVelocity.x * spinFactor;

			transform.rotation.x = std::fmod(transform.rotation.x, 360.0f);
			if (transform.rotation.x < 0.0f)
			{
				transform.rotation.x += 360.0f;
			}

			transform.rotation.z = std::fmod(transform.rotation.z, 360.0f);
			if (transform.rotation.z < 0.0f)
			{
				transform.rotation.z += 360.0f;
			}

			scene.MakeDirty<re::TransformComponent>(entity);
		}
	}

	static void KeepBallSpeedConstant(re::ecs::Scene& scene)
	{
		for (auto&& [entity, ball, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
		{
			if (ball.isAttachedToPaddle)
			{
				continue;
			}

			const re::Vector3f currentVel = rb.linearVelocity;
			const float currentSpeed = currentVel.Length();
			if (currentSpeed > std::numeric_limits<float>::epsilon())
			{
				if (std::abs(currentSpeed - ball.speed) > std::numeric_limits<float>::epsilon())
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

	static void UpdatePaddle(re::ecs::Scene& scene)
	{
		for (auto&& [entity, transform, paddle, rb] : *scene.CreateView<re::TransformComponent, PaddleComponent, re::physics::RigidBody>())
		{
			rb.linearVelocity.x = static_cast<float>(paddle.moveDir) * paddle.speed;
			rb.isVelocityDirty = true;

			const float limit = constants::FieldLimitX - (paddle.width * 0.5f);
			if (transform.position.x > limit || transform.position.x < -limit)
			{
				rb.linearVelocity = re::Vector3f::Zero();

				transform.position.x = std::clamp(transform.position.x, -limit, limit);
				rb.position = transform.position;
			}

			scene.MakeDirty<re::TransformComponent>(entity);
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

				rb.linearVelocity = re::Vector3f::Zero();
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

		for (const auto& [A, B] : collisions)
		{
			const re::ecs::Entity entityA{ A };
			const re::ecs::Entity entityB{ B };

			if (!scene.IsValid(entityA) || !scene.IsValid(entityB))
			{
				continue;
			}

			CheckBallCollision(scene, entityA, entityB);
			CheckBallCollision(scene, entityB, entityA);
		}
	}

	static void CheckBallCollision(re::ecs::Scene& scene, const re::ecs::Entity ballEntity, const re::ecs::Entity otherEntity)
	{
		if (!scene.HasComponent<BallComponent>(ballEntity))
		{
			return;
		}

		const auto& ball = scene.GetComponent<BallComponent>(ballEntity);
		auto& ballRb = scene.GetComponent<re::physics::RigidBody>(ballEntity);
		const auto& ballTrans = scene.GetComponent<re::TransformComponent>(ballEntity);

		if (scene.HasComponent<BrickComponent>(otherEntity))
		{
			auto& brick = scene.GetComponent<BrickComponent>(otherEntity);
			brick.health--;
			if (brick.health <= 0)
			{
				scene.DestroyEntity(otherEntity);
				AddScore(scene, brick.scoreValue);
			}
		}
		else if (scene.HasComponent<PaddleComponent>(otherEntity))
		{
			const auto& paddle = scene.GetComponent<PaddleComponent>(otherEntity);
			const auto& paddleTrans = scene.GetComponent<re::TransformComponent>(otherEntity);

			const float hitFactor = (ballTrans.position.x - paddleTrans.position.x) / (paddle.width * 0.5f);

			ballRb.linearVelocity.z = -std::abs(ballRb.linearVelocity.z);
			ballRb.linearVelocity.x = hitFactor * ball.speed * constants::PaddleBounceFactor;

			NormalizeSpeed(ballRb.linearVelocity, ball.speed);
			ballRb.isVelocityDirty = true;
		}
		else if (scene.HasComponent<BottomDeathZoneTag>(otherEntity))
		{
			LoseLife(scene);
		}
	}

	static void NormalizeSpeed(re::Vector3f& vel, const float targetSpeed)
	{
		if (const float currentSpeed = vel.Length(); currentSpeed > std::numeric_limits<float>::epsilon())
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
			ball.velocity = re::Vector3f::Zero();
		}
	}

	static void AddScore(re::ecs::Scene& scene, const int score)
	{
		const auto state = scene.FindComponent<GameStateComponent>();
		if (!state)
		{
			return;
		}

		state->score += score;
	}
};

} // namespace arcanoid