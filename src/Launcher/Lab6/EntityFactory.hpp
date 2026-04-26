#pragma once

#include <Core/Math/Vector3.hpp>
#include <ECS/Scene.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/System/HierarchySystem.hpp>
#include <Runtime/System/OrbitCameraSystem.hpp>

#include "BattleCityComponents.hpp"

namespace battle_city
{

class EntityFactory
{
public:
	explicit EntityFactory(re::ecs::Scene& m_scene)
		: m_scene(m_scene)
	{
	}

	void SpawnProjectile(const re::Vector3f& tankPos, const re::Vector3f& dir, const TeamType team) const
	{
		const auto assetsOpt = m_scene.FindComponent<GameAssetsComponent>();
		if (!assetsOpt)
		{
			return;
		}

		constexpr float TURRET_Y_OFFSET = 0.8f;
		constexpr float BARREL_LENGTH = TANK_SIZE * 0.7f;

		const re::Vector3f spawnPos = {
			tankPos.x + dir.x * BARREL_LENGTH,
			tankPos.y + TURRET_Y_OFFSET,
			tankPos.z + dir.z * BARREL_LENGTH
		};

		m_scene.CreateEntity()
			.Add<ProjectileComponent>({ .team = team, .speed = PROJECTILE_SPEED })
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Dynamic,
				.collider = { .type = re::physics::ColliderType::Sphere, .radius = PROJECTILE_RADIUS },
				.friction = 0.0f,
				.restitution = 0.0f,
				.gravityFactor = 0.0f,
				.linearVelocity = { dir.x * PROJECTILE_SPEED, 0.f, dir.z * PROJECTILE_SPEED },
				.lockTranslation = { false, true, false },
				.isVelocityDirty = true,
				.isSensor = true,
			})
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = spawnPos,
				.scale = { PROJECTILE_RADIUS * 2.f, PROJECTILE_RADIUS * 2.f, PROJECTILE_RADIUS * 2.f },
			})
			.Add<re::MaterialComponent>(re::Material{ .albedoMap = assetsOpt->projectileTex })
			.Add<re::StaticMeshComponent3D>(assetsOpt->sphereMesh);
	}

	void SpawnEnemy(const re::Vector3f& pos) const
	{
		const auto assetsOpt = m_scene.FindComponent<GameAssetsComponent>();
		if (!assetsOpt)
		{
			return;
		}

		const auto enemy
			= m_scene.CreateEntity()
				  .Add<TankComponent>({ .team = TeamType::Enemy, .speed = ENEMY_SPEED, .fireCooldown = 1.5f, .facingDir = { -1.f, 0.f, 0.f } })
				  .Add<EnemyAiComponent>({ .currentDir = { -1.f, 0.f, 0.f } })
				  .Add<re::RigidBodyComponent>({
					  .type = re::physics::BodyType::Dynamic,
					  .collider = { .type = re::physics::ColliderType::Box },
					  .friction = 0.0f,
					  .restitution = 0.0f,
					  .lockTranslation = { false, true, false },
					  .lockRotation = re::Vector3(true),
				  })
				  .Add<re::detail::OpaqueTag>()
				  .Add<re::Dirty<re::TransformComponent>>()
				  .Add<re::TransformComponent>({
					  .position = pos,
					  .rotation = { 0.f, 180.f, 0.f },
					  .scale = { TANK_SIZE, TANK_SIZE, TANK_SIZE },
				  });

		for (auto&& [vertices, indices, material, vao, vbo, ebo] : assetsOpt->enemyModel->GetParts())
		{
			m_scene.CreateEntity()
				.Add<re::Dirty<re::TransformComponent>>()
				.Add<re::TransformComponent>()
				.Add<re::HierarchyComponent>({ .parent = enemy.GetEntity() })
				.Add<re::detail::OpaqueTag>()
				.Add<re::MaterialComponent>(material)
				.Add<re::StaticMeshComponent3D>(vertices, indices);
		}
	}

	void SpawnPlayer(const re::Vector3f& pos) const
	{
		const auto assetsOpt = m_scene.FindComponent<GameAssetsComponent>();
		if (!assetsOpt)
		{
			return;
		}

		const auto player
			= m_scene.CreateEntity()
				  .Add<TankComponent>({ .team = TeamType::Player, .speed = PLAYER_SPEED, .fireCooldown = 0.5f, .facingDir = { 1.f, 0.f, 0.f } })
				  .Add<PlayerInputComponent>()
				  .Add<re::RigidBodyComponent>({
					  .type = re::physics::BodyType::Dynamic,
					  .collider = { .type = re::physics::ColliderType::Box },
					  .friction = 0.0f,
					  .restitution = 0.0f,
					  .lockTranslation = { false, true, false },
					  .lockRotation = re::Vector3(true),
				  })
				  .Add<re::Dirty<re::TransformComponent>>()
				  .Add<re::TransformComponent>({
					  .position = pos,
					  .scale = { TANK_SIZE, TANK_SIZE, TANK_SIZE },
				  });

		int partIndex = 0;
		for (auto&& [vertices, indices, material, vao, vbo, ebo] : assetsOpt->playerModel->GetParts())
		{
			auto child = m_scene.CreateEntity()
							 .Add<re::Dirty<re::TransformComponent>>()
							 .Add<re::TransformComponent>()
							 .Add<re::HierarchyComponent>({ .parent = player.GetEntity() })
							 .Add<re::detail::OpaqueTag>()
							 .Add<re::MaterialComponent>(material)
							 .Add<re::StaticMeshComponent3D>(vertices, indices);

			if (partIndex == 1)
			{
				child.Add<TurretComponent>();
			}
			partIndex++;
		}

#if 1
		if (auto cameraEntity = m_scene.FindFirstWith<re::CameraComponent>(); cameraEntity.IsValid())
		{
			if (!cameraEntity.Has<re::OrbitCameraComponent>())
			{
				cameraEntity.Add<re::OrbitCameraComponent>();
			}

			auto& orbit = cameraEntity.Get<re::OrbitCameraComponent>();
			orbit.target = player.GetEntity();
			orbit.distance = 25.0f;
			orbit.pitch = -50.0f;
			orbit.yaw = 0.0f;
		}
#endif
	}

private:
	re::ecs::Scene& m_scene;
};

} // namespace battle_city