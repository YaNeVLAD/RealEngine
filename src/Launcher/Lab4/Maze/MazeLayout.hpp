#pragma once

#include "MazeGame.hpp"

#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>
#include <Runtime/System/MouseLookSystem.hpp>

#include <vector>

struct MazeLayout final : re::Layout
{
	MazeLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		using namespace re::literals;

		auto& scene = GetScene();

		scene.AddSystem<MazePlayerControllerSystem>().RunOnMainThread();
		scene.AddSystem<re::MouseLookSystem>().RunOnMainThread();

		const auto [wallV, wallI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::White);
		const auto wallMesh = std::make_shared<re::StaticMesh>(wallV, wallI);

		const auto [floorV, floorI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::Navy);
		const auto floorMesh = std::make_shared<re::StaticMesh>(floorV, floorI);

		const auto [ceilV, ceilI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::Maroon);
		const auto ceilMesh = std::make_shared<re::StaticMesh>(ceilV, ceilI);

		CreateMazeMap(wallMesh, floorMesh, ceilMesh);
	}

	void OnAttach() override
	{
		m_window.SetCursorLocked(true);
	}

	void OnDetach() override
	{
		m_window.SetCursorLocked(false);
	}

	void OnEvent(re::Event const& event) override
	{
		if (!m_player.Valid())
		{
			return;
		}
		re::MouseLookSystem::OnEvent(event, GetScene());
		auto& playerState = GetScene().GetComponent<MazePlayerStateComponent>(m_player);

		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			HandlePlayerMovement(playerState, e->key, true);
		}

		if (const auto* e = event.GetIf<re::Event::KeyReleased>())
		{
			HandlePlayerMovement(playerState, e->key, false);
		}
	}

private:
	static void HandlePlayerMovement(MazePlayerStateComponent& playerState, const re::Keyboard::Key key, const bool enable)
	{
		// clang-format off
		switch (key)
		{
		case re::Keyboard::Key::W: playerState.forward = enable; break;
		case re::Keyboard::Key::S: playerState.backward = enable; break;
		case re::Keyboard::Key::A: playerState.left = enable; break;
		case re::Keyboard::Key::D: playerState.right = enable; break;
		default: break;
		}
		// clang-format on
	}

	void CreateMazeMap(
		std::shared_ptr<re::StaticMesh> wallMesh,
		std::shared_ptr<re::StaticMesh> floorMesh,
		std::shared_ptr<re::StaticMesh> ceilMesh)
	{
		auto& scene = GetScene();
		const std::vector<std::string_view> map = {
			"1111111111111111",
			"1000000000000001",
			"1011101111101101",
			"1010001000100101",
			"1010111010110101",
			"0P00000010000001",
			"1011111111111101",
			"1010000000000101",
			"1010111111110101",
			"1010100000010101",
			"1000101111010001",
			"1110101001010111",
			"1000001000000001",
			"1011111011111101",
			"1000000000000001",
			"1111111111111111"
		};

		constexpr float BLOCK_SIZE = 3.0f;
		constexpr float HALF_BLOCK = BLOCK_SIZE / 2.0f;

		for (std::size_t z = 0; z < map.size(); ++z)
		{
			for (std::size_t x = 0; x < map[z].size(); ++x)
			{
				const float worldX = x * BLOCK_SIZE;
				const float worldZ = z * BLOCK_SIZE;

				if (map[z][x] == '1')
				{
					scene.CreateEntity() // Wall
						.Add<re::ColliderComponent3D>()
						.Add<re::detail::OpaqueTag>()
						.Add<re::Dirty<re::TransformComponent>>()
						.Add<re::TransformComponent>({
							.position = { worldX, 0.0f, worldZ },
							.scale = { BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE },
						})
						.Add<MazeWallComponent>({ worldX, worldZ, HALF_BLOCK })
						.Add<re::StaticMeshComponent3D>(wallMesh);
				}
				else if (map[z][x] == 'P')
				{
					m_player = scene.CreateEntity() // Player
								   .Add<re::ColliderComponent3D>()
								   .Add<MazePlayerStateComponent>()
								   .Add<re::CameraComponent>({ .isPrimal = true, .fov = 60.f })
								   .Add<re::Dirty<re::TransformComponent>>()
								   .Add<re::MouseLookComponent>()
								   .Add<re::TransformComponent>({
									   .position = { worldX, 0.f, worldZ },
									   .rotation = { 0.f, 90.f, 0.f },
								   })
								   .Add<re::LightComponent>(re::LightComponent::CreateSpotlight(
									   re::Color{ 255, 255, 240, 255 }, // Слегка теплый свет фонарика
									   60.0f, // Угол конуса в градусах (GL_SPOT_CUTOFF) [cite: 221, 228]
									   2.0f // Концентрация света к центру (GL_SPOT_EXPONENT) [cite: 222, 229]
									   ))
								   .GetEntity();
				}

				scene.CreateEntity() // Floor
					.Add<re::ColliderComponent3D>()
					.Add<re::detail::OpaqueTag>()
					.Add<re::Dirty<re::TransformComponent>>()
					.Add<re::TransformComponent>({
						.position = { worldX, -BLOCK_SIZE, worldZ },
						.scale = { BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE },
					})
					.Add<re::StaticMeshComponent3D>(floorMesh);

				scene.CreateEntity() // Ceil
					.Add<re::ColliderComponent3D>()
					.Add<re::detail::OpaqueTag>()
					.Add<re::Dirty<re::TransformComponent>>()
					.Add<re::TransformComponent>({
						.position = { worldX, BLOCK_SIZE, worldZ },
						.scale = { BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE },
					})
					.Add<re::StaticMeshComponent3D>(ceilMesh);
			}
		}

		RE_ASSERT(m_player.Valid(), "You forgot to add a player spawn ('P') on the maze map");
	}

	re::render::IWindow& m_window;
	re::ecs::Entity m_player = re::ecs::Entity::INVALID_ID;
};