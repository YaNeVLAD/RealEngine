#pragma once

#include <RenderCore/Assets/AssetManager.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>
#include <Runtime/LayoutTraits.hpp>
#include <Runtime/System/OrbitCameraSystem.hpp>

#include "BattleCityComponents.hpp"
#include "BattleCitySystem.hpp"

#include <string_view>
#include <vector>

namespace battle_city
{

class BattleCityLayout final : public re::LayoutTraits<BattleCityLayout, re::OrbitCameraTrait>
{
public:
	BattleCityLayout(re::Application& app, re::render::IWindow& window)
		: LayoutTraits(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		scene.AddSystem<BattleCitySystem>(scene).RunOnMainThread();
		scene.AddSystem<re::OrbitCameraSystem>().RunOnMainThread();

		auto camera = scene.FindFirstWith<re::CameraComponent>();
		auto& transform = camera.Get<re::TransformComponent>();

		transform.position = { -30.f, 50.f, 0.f };
		transform.rotation = { -65.f, 0.f, 0.f };

		scene.CreateEntity()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::LightComponent>(re::LightComponent::CreateDirectional(
				re::Color::White,
				re::Color{ 80, 80, 80 }))
			.Add<re::TransformComponent>({
				.rotation = { -45.f, -45.f, 0.f },
			});

		auto [cubeV, cubeI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::White);
		const auto cubeMesh = std::make_shared<re::StaticMesh>(cubeV, cubeI);

		auto [sphereV, sphereI] = re::detail::PrimitiveBuilder::CreateSphere(re::Color::White);
		const auto sphereMesh = std::make_shared<re::StaticMesh>(sphereV, sphereI);

		scene.CreateEntity()
			.Add<GameAssetsComponent>({
				.cubeMesh = cubeMesh,
				.sphereMesh = sphereMesh,
				.playerModel = m_manager.Get<re::Model>("battle_city/Tank1.obj"),
				.enemyModel = m_manager.Get<re::Model>("battle_city/Tank1.obj"),
				.projectileTex = m_manager.Get<re::Texture>("battle_city/projectile.png"),
				.baseTex = m_manager.Get<re::Texture>("battle_city/base.png"),
			});

		m_gameStateEntity
			= scene.CreateEntity()
				  .Add<GameStateComponent>()
				  .GetEntity();

		CreateMap(cubeMesh);
	}

	void OnAttach() override
	{
		m_window.SetBackgroundColor(re::Color::Black);
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();
		const auto stateOpt = scene.FindComponent<GameStateComponent>();

		m_timeAccumulator += dt;
		if (m_timeAccumulator >= 0.5f && stateOpt)
		{
			re::String statusMsg;
			if (stateOpt->isGameOver)
			{
				statusMsg = "GAME OVER";
			}
			else if (stateOpt->isVictory)
			{
				statusMsg = "VICTORY!";
			}
			else
			{
				statusMsg = std::format("Battle City | Lives: {} | Score: {} | Enemies Left: {}",
					stateOpt->lives,
					stateOpt->score,
					stateOpt->enemiesToSpawn + stateOpt->enemiesActive);
			}

			m_window.SetTitle(statusMsg);
			m_timeAccumulator = 0.0f;
		}
	}

	void OnLayoutEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			HandleInput(e->key, true);
		}
		if (const auto* e = event.GetIf<re::Event::KeyReleased>())
		{
			HandleInput(e->key, false);
		}
	}

private:
	void HandleInput(const re::Keyboard::Key key, const bool isPressed)
	{
		for (auto&& [entity, input] : *GetScene().CreateView<PlayerInputComponent>())
		{
			if (key == re::Keyboard::Key::W || key == re::Keyboard::Key::Up)
			{
				input.up = isPressed;
			}
			if (key == re::Keyboard::Key::S || key == re::Keyboard::Key::Down)
			{
				input.down = isPressed;
			}
			if (key == re::Keyboard::Key::A || key == re::Keyboard::Key::Left)
			{
				input.left = isPressed;
			}
			if (key == re::Keyboard::Key::D || key == re::Keyboard::Key::Right)
			{
				input.right = isPressed;
			}
			if (key == re::Keyboard::Key::Space)
			{
				input.shoot = isPressed;
			}
		}
	}

	void CreateMap(std::shared_ptr<re::StaticMesh> cubeMesh)
	{
		auto& scene = GetScene();
		const auto assetsOpt = scene.FindComponent<GameAssetsComponent>();

		const std::vector<std::string_view> mapLayout = {
			"E00000E00000E",
			"0000000000000",
			"0B0B0B0B0B0B0",
			"0B0B0B0B0B0B0",
			"0B0B0B0B0B0B0",
			"0000000000000",
			"0B0B0AWA0A0B0",
			"0T0B0WWW000B0",
			"0IIIIAWAIIII0",
			"0B0B0B0B0B0B0",
			"0000000000000",
			"00000BAB00000",
			"00000PXB00000",
		};

		const auto brickTex = m_manager.Get<re::Texture>("battle_city/brick.png");
		const auto armorTex = m_manager.Get<re::Texture>("battle_city/armor.jpg");
		const auto waterTex = m_manager.Get<re::Texture>("battle_city/water.jpg");
		const auto iceTex = m_manager.Get<re::Texture>("battle_city/ice.jpg");
		const auto treeTex = m_manager.Get<re::Texture>("battle_city/tree.jpg");
		const auto groundTex = m_manager.Get<re::Texture>("battle_city/ground.jpg");

		scene.CreateEntity()
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = { 0.f, -0.3f, 0.f },
				.scale = { 13.0f * BLOCK_SIZE, 0.1f, 13.0f * BLOCK_SIZE },
			})
			.Add<re::MaterialComponent>(re::Material{ .albedoMap = groundTex })
			.Add<re::StaticMeshComponent3D>(cubeMesh);

		constexpr float boundsOffset = 13.0f * BLOCK_SIZE * 0.5f + 0.5f;
		constexpr float boundsLength = 13.0f * BLOCK_SIZE + 2.0f;
		CreateBorder(scene, { boundsOffset, 0.5f, 0.f }, { 0.5f, 1.0f, boundsLength * 0.5f }); // Top (+X)
		CreateBorder(scene, { -boundsOffset, 0.5f, 0.f }, { 0.5f, 1.0f, boundsLength * 0.5f }); // Bottom (-X)
		CreateBorder(scene, { 0.f, 0.5f, boundsOffset }, { boundsLength * 0.5f, 1.0f, 0.5f }); // Right (+Z)
		CreateBorder(scene, { 0.f, 0.5f, -boundsOffset }, { boundsLength * 0.5f, 1.0f, 0.5f }); // Left (-Z)

		for (std::size_t row = 0; row < mapLayout.size(); ++row)
		{
			for (std::size_t col = 0; col < mapLayout[row].size(); ++col)
			{
				const float worldX = (6.0f - static_cast<float>(row)) * BLOCK_SIZE;
				const float worldZ = (static_cast<float>(col) - 6.0f) * BLOCK_SIZE;

				if (const char tile = mapLayout[row][col]; tile == 'B')
				{
					SpawnBlock(scene, cubeMesh, brickTex, BlockType::Brick, { worldX, 1.f, worldZ }, false);
				}
				else if (tile == 'A')
				{
					SpawnBlock(scene, cubeMesh, armorTex, BlockType::Armor, { worldX, 1.f, worldZ }, false);
				}
				else if (tile == 'T')
				{
					SpawnBlock(scene, cubeMesh, treeTex, BlockType::Tree, { worldX, 1.f, worldZ }, false);
				}
				else if (tile == 'W')
				{
					SpawnBlock(scene, cubeMesh, waterTex, BlockType::Water, { worldX, -0.25f, worldZ }, false, 0.25f);
				}
				else if (tile == 'I')
				{
					SpawnBlock(scene, cubeMesh, iceTex, BlockType::Ice, { worldX, -0.25f, worldZ }, true, 0.25f);
				}
				else if (tile == 'E')
				{
					scene.CreateEntity()
						.Add<EnemySpawnPointComponent>()
						.Add<re::TransformComponent>({ .position = { worldX, -0.25f, worldZ } });
				}
				else if (tile == 'P')
				{
					auto& state = scene.GetComponent<GameStateComponent>(m_gameStateEntity);
					state.playerSpawnPos = { worldX, -0.25f, worldZ };

					scene.CreateEntity()
						.Add<PlayerSpawnPointComponent>()
						.Add<re::TransformComponent>({ .position = state.playerSpawnPos });
				}
				else if (tile == 'X')
				{
					scene.CreateEntity()
						.Add<BaseComponent>()
						.Add<re::RigidBodyComponent>({
							.type = re::physics::BodyType::Static,
							.collider = { re::physics::ColliderType::Box },
						})
						.Add<re::detail::OpaqueTag>()
						.Add<re::Dirty<re::TransformComponent>>()
						.Add<re::TransformComponent>({
							.position = { worldX, 1.f, worldZ },
							.scale = { BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE },
						})
						.Add<re::MaterialComponent>(re::Material{ .albedoMap = assetsOpt ? assetsOpt->baseTex : nullptr })
						.Add<re::StaticMeshComponent3D>(cubeMesh);
				}
			}
		}
	}

	static void SpawnBlock(re::ecs::Scene& scene, std::shared_ptr<re::StaticMesh> mesh, const std::shared_ptr<re::Texture>& texture,
		const BlockType type, const re::Vector3f pos, const bool isSensor, const float height = BLOCK_SIZE)
	{
		scene.CreateEntity()
			.Add<BlockComponent>({ .type = type })
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box },
				.isSensor = isSensor,
			})
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = pos,
				.scale = { BLOCK_SIZE, height, BLOCK_SIZE },
			})
			.Add<re::MaterialComponent>(re::Material{ .albedoMap = texture })
			.Add<re::StaticMeshComponent3D>(mesh);
	}

	static void CreateBorder(re::ecs::Scene& scene, const re::Vector3f& pos, const re::Vector3f& halfExtents)
	{
		scene.CreateEntity()
			.Add<re::TransformComponent>({ .position = pos })
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box, halfExtents },
			});
	}

	float m_timeAccumulator = 0.0f;

	re::render::IWindow& m_window;
	re::AssetManager m_manager;
	re::ecs::Entity m_gameStateEntity = re::ecs::Entity::INVALID_ID;
};

} // namespace battle_city