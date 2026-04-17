#pragma once

#include "BonusFactory.hpp"

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
	ArcanoidGameSystem() = default;

	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		UpdateTimers(scene, dt);
		UpdatePaddle(scene);
		HandleCollisions(scene);
		UpdateBallAttached(scene);
		ClampBallSpeed(scene);
		AnimateBallRotation(scene, dt);
	}

private:
	BonusFactory m_bonusFactory;

	static void UpdateTimers(re::ecs::Scene& scene, const float dt)
	{
		const auto stateComp = scene.FindComponent<GameStateComponent>();
		if (!stateComp)
		{
			return;
		}

		if (stateComp->paddleSizeTimer > 0.0f)
		{
			stateComp->paddleSizeTimer -= dt;
			if (stateComp->paddleSizeTimer <= 0.0f)
			{
				ResetPaddleSize(scene);
			}
		}

		if (stateComp->ballSpeedTimer > 0.0f)
		{
			stateComp->ballSpeedTimer -= dt;
			if (stateComp->ballSpeedTimer <= 0.0f)
			{
				ResetBallSpeed(scene);
			}
		}
	}

	static void ResetPaddleSize(re::ecs::Scene& scene)
	{
		for (auto&& [entity, transform, paddle] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
		{
			paddle.width = constants::PaddleWidth;
			transform.scale.x = paddle.width;
			scene.MakeDirty<re::TransformComponent>(entity);
		}
	}

	static void ResetBallSpeed(re::ecs::Scene& scene)
	{
		for (auto&& [entity, ball, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
		{
			ball.speed = constants::BallSpeed;
			NormalizeSpeed(rb.linearVelocity, ball.speed);
			rb.isVelocityDirty = true;
		}
	}

	static void AnimateBallRotation(re::ecs::Scene& scene, const float dt)
	{
		for (auto&& [entity, transform, ball, rb] : *scene.CreateView<re::TransformComponent, BallComponent, re::physics::RigidBody>())
		{
			if (ball.isAttachedToPaddle)
			{
				continue;
			}

			// Сделать вращение шара зависимым от направления его текущего движения
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

	static void ClampBallSpeed(re::ecs::Scene& scene)
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

	void HandleCollisions(re::ecs::Scene& scene)
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

			CheckBonusCollision(scene, entityA, entityB);
			CheckBonusCollision(scene, entityB, entityA);
		}
	}

	void CheckBallCollision(re::ecs::Scene& scene, const re::ecs::Entity ballEntity, const re::ecs::Entity otherEntity)
	{
		if (!scene.HasComponent<BallComponent>(ballEntity))
		{
			return;
		}

		const auto& ball = scene.GetComponent<BallComponent>(ballEntity);
		if (ball.isAttachedToPaddle)
		{
			return;
		}

		auto& ballRb = scene.GetComponent<re::physics::RigidBody>(ballEntity);
		const auto& ballTrans = scene.GetComponent<re::TransformComponent>(ballEntity);

		if (scene.HasComponent<BrickComponent>(otherEntity))
		{
			auto& brick = scene.GetComponent<BrickComponent>(otherEntity);
			const auto brickTransform = scene.GetComponent<re::TransformComponent>(otherEntity);
			brick.health--;
			if (brick.health <= 0)
			{
				scene.DestroyEntity(otherEntity);
				AddScore(scene, brick.scoreValue);
			}

			if (static_cast<float>(std::rand()) / RAND_MAX < constants::BonusDropChance)
			{
				m_bonusFactory.Spawn(scene, brickTransform.position);
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
			int activeBallsCount = 0;
			for (auto&& [_, __] : *scene.CreateView<BallComponent>())
			{
				activeBallsCount++;
			}

			if (activeBallsCount > 1)
			{
				scene.DestroyEntity(ballEntity);
			}
			else
			{
				LoseLife(scene);
			}
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
		const auto state = scene.FindComponent<GameStateComponent>();
		if (state)
		{
			state->lives--;
			if (state->lives <= 0)
			{
				state->lives = constants::StartingLives;
				state->score = 0;
				state->currentLevel = 1;
				state->requestNextLevel = true;
			}
			state->paddleSizeTimer = 0.0f;
			state->ballSpeedTimer = 0.0f;
		}

		ResetPaddleSize(scene);
		ResetBallSpeed(scene);

		for (auto&& [entity, ball, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
		{
			ball.isAttachedToPaddle = true;

			rb.linearVelocity = re::Vector3f::Zero();
			rb.isVelocityDirty = true;
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

	static void CheckBonusCollision(re::ecs::Scene& scene, const re::ecs::Entity bonusEntity, const re::ecs::Entity otherEntity)
	{
		if (!scene.HasComponent<BonusComponent>(bonusEntity))
		{
			return;
		}

		const auto& [type] = scene.GetComponent<BonusComponent>(bonusEntity);

		if (scene.HasComponent<PaddleComponent>(otherEntity))
		{
			ApplyBonus(scene, type);
			scene.DestroyEntity(bonusEntity);
		}
		else if (scene.HasComponent<BottomDeathZoneTag>(otherEntity))
		{
			scene.DestroyEntity(bonusEntity);
		}
	}

	static void ApplyBonus(re::ecs::Scene& scene, const BonusType type)
	{
		const auto state = scene.FindComponent<GameStateComponent>();
		if (!state)
		{
			return;
		}

		switch (type)
		{
		case BonusType::ExtraLife:
			state->lives++;
			break;
		case BonusType::Expand:
			state->paddleSizeTimer = constants::PowerupDuration;
			for (auto&& [e, t, p] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
			{
				p.width = constants::PaddleWidthExpanded;
				t.scale.x = p.width;
				scene.MakeDirty<re::TransformComponent>(e);
			}
			break;
		case BonusType::Shrink:
			state->paddleSizeTimer = constants::PowerupDuration;
			for (auto&& [e, t, p] : *scene.CreateView<re::TransformComponent, PaddleComponent>())
			{
				p.width = constants::PaddleWidthShrunk;
				t.scale.x = p.width;
				scene.MakeDirty<re::TransformComponent>(e);
			}
			break;
		case BonusType::Slow:
			state->ballSpeedTimer = constants::PowerupDuration;
			for (auto&& [e, b, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
			{
				b.speed = constants::BallSpeedSlow;
				NormalizeSpeed(rb.linearVelocity, b.speed);
				rb.isVelocityDirty = true;
			}
			break;
		case BonusType::Fast:
			state->ballSpeedTimer = constants::PowerupDuration;
			for (auto&& [e, b, rb] : *scene.CreateView<BallComponent, re::physics::RigidBody>())
			{
				b.speed = constants::BallSpeedFast;
				NormalizeSpeed(rb.linearVelocity, b.speed);
				rb.isVelocityDirty = true;
			}
			break;
		case BonusType::MultiBall:
			const auto balls = scene.FindEntitiesWith<BallComponent>();
			if (balls.size() <= 64)
			{
				SpawnMultiBalls(scene);
			}
			break;
		}
	}

	static void SpawnMultiBalls(re::ecs::Scene& scene)
	{
		struct BallData
		{
			re::TransformComponent t;
			BallComponent b;
			re::Vector3f vel;
			std::shared_ptr<re::StaticMesh> mesh;
			re::MaterialComponent mat;
		};
		std::vector<BallData> clones;

		for (auto&& [e, t, b, rb, sm, mat] : *scene.CreateView<re::TransformComponent, BallComponent, re::physics::RigidBody, re::StaticMeshComponent3D, re::MaterialComponent>())
		{
			if (b.isAttachedToPaddle)
			{
				continue;
			}

			BallData d1 = { t, b, rb.linearVelocity, sm.mesh, mat };
			BallData d2 = { t, b, rb.linearVelocity, sm.mesh, mat };

			NormalizeSpeed(d1.vel, b.speed);
			NormalizeSpeed(d2.vel, b.speed);

			clones.push_back(d1);
			clones.push_back(d2);
		}

		for (const auto& [transform, ball, vel, mesh, material] : clones)
		{
			scene.CreateEntity()
				.Add<BallComponent>(ball)
				.Add<re::RigidBodyComponent>({
					.type = re::physics::BodyType::Dynamic,
					.collider = { .type = re::physics::ColliderType::Sphere, .radius = constants::BallRadius },
					.friction = 0.0f,
					.restitution = 1.0f,
					.gravityFactor = 0.0f,
					.linearDamping = 0.f,
					.linearVelocity = vel,
					.lockTranslation = { false, true, false },
					.lockRotation = re::Vector3(true),
					.isVelocityDirty = true,
				})
				.Add<re::detail::OpaqueTag>()
				.Add<re::Dirty<re::TransformComponent>>()
				.Add<re::TransformComponent>(transform)
				.Add<re::MaterialComponent>(material)
				.Add<re::StaticMeshComponent3D>(mesh);
		}
	}
};

} // namespace arcanoid