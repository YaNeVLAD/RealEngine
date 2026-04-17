#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include "ArcanoidGameSystem.hpp"
#include "Components.hpp"
#include "Constants.hpp"

class ArcanoidLayout final : public re::Layout
{
public:
	ArcanoidLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		scene.AddSystem<arcanoid::ArcanoidGameSystem>().RunOnMainThread();

		m_gameStateEntity = scene.CreateEntity()
								.Add<arcanoid::GameStateComponent>()
								.GetEntity();

		auto camera = scene.FindFirstWith<re::CameraComponent>();
		auto& transform = camera.Get<re::TransformComponent>();

		transform.position = arcanoid::constants::CameraPos;
		transform.rotation = arcanoid::constants::CameraRot;

		scene.CreateEntity()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::LightComponent>(re::LightComponent::CreateDirectional(
				re::Color::White,
				re::Color{ 80, 80, 80 }))
			.Add<re::TransformComponent>({
				.position = arcanoid::constants::LightPos,
				.rotation = arcanoid::constants::LightRot,
			});

		auto [cubeV, cubeI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::White);
		m_cubeMesh = std::make_shared<re::StaticMesh>(cubeV, cubeI);

		// Поизучать как тестуры натянуть на сферу
		auto [sphereV, sphereI] = re::detail::PrimitiveBuilder::CreateSphere(re::Color::White);
		m_sphereMesh = std::make_shared<re::StaticMesh>(sphereV, sphereI);

		CreateBackground();
		CreatePlayer();
		LoadLevel(1);
	}

	void OnAttach() override
	{
		m_window.SetBackgroundColor(re::Color::Black);
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();
		auto& state = scene.GetComponent<arcanoid::GameStateComponent>(m_gameStateEntity);

		if (const bool hasBricks = scene.ContainsAny<arcanoid::BrickComponent>();
			!hasBricks || state.requestNextLevel)
		{
			state.requestNextLevel = false;
			if (!hasBricks)
			{
				state.currentLevel++;
			}
			LoadLevel(state.currentLevel);
		}

		m_timeAccumulator += dt;
		if (m_timeAccumulator >= 1.0f)
		{
			const auto title = std::format("Arcanoid | Level: {} | Score: {} | Lives: {}",
				state.currentLevel,
				state.score,
				state.lives);

			m_window.SetTitle(title);
			m_timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		auto& scene = GetScene();

		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			HandleInput(e->key, true);

			if (e->key == re::Keyboard::Key::Space)
			{
				for (auto&& [entity, ball, rb] : *scene.CreateView<arcanoid::BallComponent, re::RigidBodyComponent>())
				{
					if (ball.isAttachedToPaddle)
					{
						ball.isAttachedToPaddle = false;
						rb.linearVelocity = { 0.0f, 0.0f, -ball.speed };
						rb.isVelocityDirty = true;
					}
				}
			}
		}
		if (const auto* e = event.GetIf<re::Event::KeyReleased>())
		{
			HandleInput(e->key, false);
		}
	}

private:
	void HandleInput(const re::Keyboard::Key key, const bool isPressed)
	{
		for (auto&& [entity, paddle] : *GetScene().CreateView<arcanoid::PaddleComponent>())
		{
			if (key == re::Keyboard::Key::Left || key == re::Keyboard::Key::A)
			{
				paddle.moveDir = isPressed ? -1 : (paddle.moveDir == -1 ? 0 : paddle.moveDir);
			}
			if (key == re::Keyboard::Key::Right || key == re::Keyboard::Key::D)
			{
				paddle.moveDir = isPressed ? 1 : (paddle.moveDir == 1 ? 0 : paddle.moveDir);
			}
		}
	}

	void CreatePlayer()
	{
		auto& scene = GetScene();
		using namespace arcanoid::constants;

		const auto paddleTex = m_manager.Get<re::Texture>("arcanoid/paddle.png");
		const auto ballTex = m_manager.Get<re::Texture>("arcanoid/ball.png");

		scene.CreateEntity()
			.Add<arcanoid::PaddleComponent>()
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Kinematic,
				.collider = { re::physics::ColliderType::Box },
				.friction = 0.0f,
				.restitution = 1.0f,
			})
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = PaddleSpawnPos,
				.scale = { PaddleWidth, PaddleHeight, PaddleDepth },
			})
			.Add<re::MaterialComponent>(re::Material{ .texture = paddleTex })
			.Add<re::StaticMeshComponent3D>(m_cubeMesh);

		scene.CreateEntity()
			.Add<arcanoid::BallComponent>()
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Dynamic,
				.collider = { .type = re::physics::ColliderType::Sphere, .radius = BallRadius },
				.friction = 0.0f,
				.restitution = 1.0f,
				.gravityFactor = 0.0f,
				.linearDamping = 0.f,
				.lockTranslation = { false, true, false },
				.lockRotation = re::Vector3(true),
			})
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = BallSpawnPos,
				.rotation = { 0.f, 135.f, 0.f },
				.scale = { BallRadius * 2.f, BallRadius * 2.f, BallRadius * 2.f },
			})
			.Add<re::MaterialComponent>(re::Material{ .texture = ballTex })
			.Add<re::StaticMeshComponent3D>(m_sphereMesh);
	}

	void CreateBackground()
	{
		auto& scene = GetScene();
		const auto bgTex = m_manager.Get<re::Texture>("arcanoid/background.png");

		scene.CreateEntity()
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = arcanoid::constants::BgPos,
				.rotation = { 5.f, 180.f, 0.f },
				.scale = arcanoid::constants::BgScale,
			})
			.Add<re::MaterialComponent>(re::Material{ .texture = bgTex })
			.Add<re::StaticMeshComponent3D>(m_cubeMesh);

		constexpr float WALL_THICKNESS = 5.0f;

		scene.CreateEntity()
			.Add<re::TransformComponent>({
				.position = { -arcanoid::constants::FieldLimitX - WALL_THICKNESS * 0.5f, 0.f, 0.f },
			})
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box, { WALL_THICKNESS * 0.5f, 5.f, 50.f } },
				.friction = 0.f,
				.restitution = 1.0f,
			});

		scene.CreateEntity()
			.Add<re::TransformComponent>({
				.position = { arcanoid::constants::FieldLimitX + WALL_THICKNESS * 0.5f, 0.f, 0.f },
			})
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box, { WALL_THICKNESS * 0.5f, 5.f, 50.f } },
				.friction = 0.f,
				.restitution = 1.0f,
			});

		scene.CreateEntity()
			.Add<re::TransformComponent>({
				.position = { 0.f, 0.f, arcanoid::constants::FieldTopZ - WALL_THICKNESS * 0.5f },
			})
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box, { 50.f, 5.f, WALL_THICKNESS * 0.5f } },
				.friction = 0.f,
				.restitution = 1.0f,
			});

		scene.CreateEntity()
			.Add<arcanoid::BottomDeathZoneTag>()
			.Add<re::TransformComponent>({
				.position = { 0.f, 0.f, arcanoid::constants::FieldBottomZ + 2.0f },
			})
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Static,
				.collider = { re::physics::ColliderType::Box, { 50.f, 5.f, 2.f } },
				.isSensor = true,
			});
	}

	void LoadLevel(const int level = 1)
	{
		using namespace arcanoid::constants;

		auto& scene = GetScene();
		for (auto&& [entity, brick] : *scene.CreateView<arcanoid::BrickComponent>())
		{
			scene.DestroyEntity(entity);
		}

		for (auto&& [entity, ball] : *scene.CreateView<arcanoid::BallComponent>())
		{
			ball.isAttachedToPaddle = true;
		}

		for (auto&& [entity, bonus] : *scene.CreateView<arcanoid::BonusComponent>())
		{
			scene.DestroyEntity(entity);
		}

		for (auto&& [entity, ball, rb] : *scene.CreateView<arcanoid::BallComponent, re::RigidBodyComponent>())
		{
			ball.isAttachedToPaddle = true;
			rb.linearVelocity = re::Vector3f::Zero();
			rb.isVelocityDirty = true;
		}

		const auto tex1 = m_manager.Get<re::Texture>("arcanoid/brick_hp1.png");
		const auto tex2 = m_manager.Get<re::Texture>("arcanoid/brick_hp2.png");

		const int rows = BaseRows + level;

		for (int row = 0; row < rows; ++row)
		{
			for (int col = 0; col < MaxCols; ++col)
			{
				const int hp = (row > 0 && row % 2 == 0) ? 2 : 1;
				const auto currentTex = (hp == 2) ? tex2 : tex1;

				scene.CreateEntity()
					.Add<arcanoid::BrickComponent>({ .health = hp })
					.Add<re::RigidBodyComponent>({
						.type = re::physics::BodyType::Static,
						.collider = { re::physics::ColliderType::Box, { BrickWidth * 0.5f, BrickHeight * 0.5f, BrickDepth * 0.5f } },
						.friction = 0.0f,
						.restitution = 1.0f,
					})
					.Add<re::detail::OpaqueTag>()
					.Add<re::Dirty<re::TransformComponent>>()
					.Add<re::TransformComponent>({
						.position = { LevelStartX + col * SpacingX, 0.f, LevelStartZ + row * SpacingZ },
						.scale = { BrickWidth, BrickHeight, BrickDepth },
					})
					.Add<re::MaterialComponent>(re::Material{ .texture = currentTex })
					.Add<re::StaticMeshComponent3D>(m_cubeMesh);
			}
		}
	}

	float m_timeAccumulator = 0.0f;

	re::render::IWindow& m_window;
	re::AssetManager m_manager;
	std::shared_ptr<re::StaticMesh> m_cubeMesh;
	std::shared_ptr<re::StaticMesh> m_sphereMesh;
	re::ecs::Entity m_gameStateEntity = re::ecs::Entity::INVALID_ID;
};