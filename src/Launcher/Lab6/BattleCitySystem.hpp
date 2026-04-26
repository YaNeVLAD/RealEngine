#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Physics/Types.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/System/HierarchySystem.hpp>

#include "BattleCityComponents.hpp"
#include "EntityFactory.hpp"

#include <cstdlib>
#include <vector>

namespace battle_city
{

class BattleCitySystem final : public re::ecs::System
{
public:
	explicit BattleCitySystem(re::ecs::Scene& scene)
		: m_entityFactory(scene)
	{
	}

	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		const auto stateOpt = scene.FindComponent<GameStateComponent>();
		if (!stateOpt || stateOpt->isGameOver || stateOpt->isVictory)
		{
			return;
		}

		UpdateIceDetection(scene);
		HandlePlayerInput(scene, dt);
		HandleEnemyAi(scene, dt);
		HandleSpawners(scene, dt);
		HandleCooldowns(scene, dt);
		ProcessCollisions(scene);
	}

private:
	EntityFactory m_entityFactory;

	static void UpdateIceDetection(re::ecs::Scene& scene)
	{
		for (auto&& [entity, trans, tank] : *scene.CreateView<re::TransformComponent, TankComponent>())
		{
			tank.onIce = false;
			for (auto&& [bEntity, bTrans, block] : *scene.CreateView<re::TransformComponent, BlockComponent>())
			{
				if (block.type == BlockType::Ice)
				{
					const float dx = std::abs(trans.position.x - bTrans.position.x);
					const float dz = std::abs(trans.position.z - bTrans.position.z);
					if (dx < BLOCK_SIZE * 0.5f && dz < BLOCK_SIZE * 0.5f)
					{
						tank.onIce = true;
						break;
					}
				}
			}
		}
	}

	void HandlePlayerInput(re::ecs::Scene& scene, const float dt) const
	{
		for (auto&& [entity, trans, tank, input, rb] : *scene.CreateView<re::TransformComponent, TankComponent, PlayerInputComponent, re::RigidBodyComponent>())
		{
			re::Vector3f targetVel = { 0.f, 0.f, 0.f };

			if (input.up)
			{
				targetVel = { tank.speed, 0.f, 0.f };
				tank.facingDir = { 1.f, 0.f, 0.f };
				trans.rotation = { 0.f, 0.f, 0.f };
			}
			else if (input.down)
			{
				targetVel = { -tank.speed, 0.f, 0.f };
				tank.facingDir = { -1.f, 0.f, 0.f };
				trans.rotation = { 0.f, 180.f, 0.f };
			}
			else if (input.left)
			{
				targetVel = { 0.f, 0.f, -tank.speed };
				tank.facingDir = { 0.f, 0.f, -1.f };
				trans.rotation = { 0.f, 90.f, 0.f };
			}
			else if (input.right)
			{
				targetVel = { 0.f, 0.f, tank.speed };
				tank.facingDir = { 0.f, 0.f, 1.f };
				trans.rotation = { 0.f, -90.f, 0.f };
			}

			ApplyVelocity(rb, targetVel, tank.onIce, dt);
			scene.MakeDirty<re::TransformComponent>(entity);

			if (input.shoot && tank.currentCooldown <= 0.0f)
			{
				m_entityFactory.SpawnProjectile(trans.position, tank.facingDir, tank.team);
				tank.currentCooldown = tank.fireCooldown;
			}
		}
	}

	void HandleEnemyAi(re::ecs::Scene& scene, const float dt) const
	{
		for (auto&& [entity, trans, tank, ai, rb] : *scene.CreateView<re::TransformComponent, TankComponent, EnemyAiComponent, re::RigidBodyComponent>())
		{
			ai.directionTimer -= dt;
			ai.fireTimer -= dt;

			if (ai.directionTimer <= 0.0f || rb.linearVelocity.Length() < 0.1f)
			{
				ai.directionTimer = 1.0f + static_cast<float>(std::rand() % 200) / 100.0f;

				if (const int randomDir = std::rand() % 4; randomDir == 0)
				{
					ai.currentDir = { 1.f, 0.f, 0.f };
					trans.rotation = { 0.f, 0.f, 0.f };
				}
				else if (randomDir == 1)
				{
					ai.currentDir = { -1.f, 0.f, 0.f };
					trans.rotation = { 0.f, 180.f, 0.f };
				}
				else if (randomDir == 2)
				{
					ai.currentDir = { 0.f, 0.f, -1.f };
					trans.rotation = { 0.f, 90.f, 0.f };
				}
				else
				{
					ai.currentDir = { 0.f, 0.f, 1.f };
					trans.rotation = { 0.f, -90.f, 0.f };
				}
				tank.facingDir = ai.currentDir;
			}

			const re::Vector3f targetVel = { ai.currentDir.x * tank.speed, 0.f, ai.currentDir.z * tank.speed };
			ApplyVelocity(rb, targetVel, tank.onIce, dt);
			scene.MakeDirty<re::TransformComponent>(entity);

			if (ai.fireTimer <= 0.0f && tank.currentCooldown <= 0.0f)
			{
				m_entityFactory.SpawnProjectile(trans.position, tank.facingDir, tank.team);
				tank.currentCooldown = tank.fireCooldown;
				ai.fireTimer = 1.0f + static_cast<float>(std::rand() % 200) / 100.0f;
			}
		}
	}

	static void ApplyVelocity(re::RigidBodyComponent& rb, const re::Vector3f& targetVel, const bool onIce, const float dt)
	{
		if (onIce)
		{
			rb.linearVelocity.x += (targetVel.x - rb.linearVelocity.x) * dt * 2.5f;
			rb.linearVelocity.z += (targetVel.z - rb.linearVelocity.z) * dt * 2.5f;
		}
		else
		{
			rb.linearVelocity.x = targetVel.x;
			rb.linearVelocity.z = targetVel.z;
		}
		rb.isVelocityDirty = true;
	}

	static void HandleCooldowns(re::ecs::Scene& scene, const float dt)
	{
		for (auto&& [entity, tank] : *scene.CreateView<TankComponent>())
		{
			if (tank.currentCooldown > 0.0f)
			{
				tank.currentCooldown -= dt;
			}
		}
	}

	void HandleSpawners(re::ecs::Scene& scene, const float dt) const
	{
		const auto stateOpt = scene.FindComponent<GameStateComponent>();
		if (!stateOpt)
		{
			return;
		}

		for (auto&& [entity, transform, spawner] : *scene.CreateView<re::TransformComponent, PlayerSpawnPointComponent>())
		{
			m_entityFactory.SpawnPlayer(transform.position);
			scene.DestroyEntity(entity);
		}

		if (stateOpt->enemiesToSpawn > 0 && stateOpt->enemiesActive < 4)
		{
			stateOpt->spawnTimer -= dt;
			if (stateOpt->spawnTimer <= 0.0f)
			{
				std::vector<re::Vector3f> spawnPoints;
				for (auto&& [entity, trans, spawner] : *scene.CreateView<re::TransformComponent, EnemySpawnPointComponent>())
				{
					spawnPoints.push_back(trans.position);
				}

				if (!spawnPoints.empty())
				{
					const auto& spawnPos = spawnPoints[std::rand() % spawnPoints.size()];
					m_entityFactory.SpawnEnemy(spawnPos);
					stateOpt->enemiesToSpawn--;
					stateOpt->enemiesActive++;
					stateOpt->spawnTimer = 3.0f;
				}
			}
		}
		else if (stateOpt->enemiesToSpawn == 0 && stateOpt->enemiesActive == 0)
		{
			stateOpt->isVictory = true;
		}
	}

	static void ProcessCollisions(re::ecs::Scene& scene)
	{
		const auto eventView = scene.CreateView<re::PhysicsEventsComponent>();
		if (eventView->begin() == eventView->end())
		{
			return;
		}

		const auto stateOpt = scene.FindComponent<GameStateComponent>();
		auto& [collisions] = scene.GetComponent<re::PhysicsEventsComponent>(std::get<0>(*eventView->begin()));

		for (const auto& [A, B] : collisions)
		{
			const re::ecs::Entity entityA{ A };
			const re::ecs::Entity entityB{ B };

			if (!scene.IsValid(entityA) || !scene.IsValid(entityB))
			{
				continue;
			}

			CheckProjectileCollision(scene, entityA, entityB, stateOpt);
			CheckProjectileCollision(scene, entityB, entityA, stateOpt);
		}
	}

	static void CheckProjectileCollision(re::ecs::Scene& scene, const re::ecs::Entity projEntity, const re::ecs::Entity otherEntity, GameStateComponent* state)
	{
		if (!scene.HasComponent<ProjectileComponent>(projEntity))
		{
			return;
		}

		const auto& [team, speed] = scene.GetComponent<ProjectileComponent>(projEntity);

		if (scene.HasComponent<BlockComponent>(otherEntity))
		{
			auto& [type, health] = scene.GetComponent<BlockComponent>(otherEntity);

			if (type == BlockType::Water || type == BlockType::Ice)
			{
				return;
			}

			if (type == BlockType::Brick || type == BlockType::Tree)
			{
				health--;
				if (health <= 0)
				{
					scene.DestroyEntity(otherEntity);
				}
				scene.DestroyEntity(projEntity);
			}
			else if (type == BlockType::Armor)
			{
				scene.DestroyEntity(projEntity);
			}
		}
		else if (scene.HasComponent<TankComponent>(otherEntity))
		{
			if (const auto& tank = scene.GetComponent<TankComponent>(otherEntity); team != tank.team)
			{
				scene.DestroyEntity(projEntity);
				scene.DestroyEntity(otherEntity);

				if (tank.team == TeamType::Enemy && state)
				{
					state->enemiesActive--;
					state->score += 100;
				}
				else if (tank.team == TeamType::Player && state)
				{
					state->lives--;
					if (state->lives > 0)
					{
						scene.CreateEntity()
							.Add<PlayerSpawnPointComponent>()
							.Add<re::TransformComponent>({ .position = state->playerSpawnPos });
					}
					else
					{
						state->isGameOver = true;
					}
				}
			}
		}
		else if (scene.HasComponent<BaseComponent>(otherEntity) && team != TeamType::Player)
		{
			scene.DestroyEntity(otherEntity);
			scene.DestroyEntity(projEntity);
			if (state)
			{
				state->isGameOver = true;
			}
		}
		else if (scene.HasComponent<ProjectileComponent>(otherEntity))
		{
			scene.DestroyEntity(projEntity);
			scene.DestroyEntity(otherEntity);
		}
	}
};

} // namespace battle_city