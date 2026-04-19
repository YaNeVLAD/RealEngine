#pragma once

#include <Core/Math/Vector3.hpp>
#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/Model.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include "Lab3/Asteroids/AsteroidsLayout.hpp"
#include "Lab4/Arcanoid/ArcanoidLayout.hpp"
#include "Lab4/Maze/MazeLayout.hpp"
#include "Lab4/Piano/PianoLayout.hpp"

#include <imgui.h>

#include "CameraControlSystem.hpp"

#include <deque>

struct MenuLayout final : re::Layout
{
	MenuLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		scene
			.AddSystem<CameraControlSystem>()
			.WithRead<re::CameraComponent, re::TransformComponent>()
			.WithWrite<re::CameraComponent>()
			.RunOnMainThread();

		scene.CreateEntity()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::LightComponent>(re::LightComponent::CreateDirectional(
				re::Color{ 255, 240, 220, 255 },
				re::Color{ 80, 80, 80, 255 }))
			.Add<re::TransformComponent>({
				.rotation = { -45.f, 45.f, 0.f },
			});

		auto camera = scene.FindFirstWith<re::CameraComponent>();
		auto& cc = camera.Get<re::CameraComponent>();
		cc.farClip = 1'000'000.f;
		camera
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Dynamic,
				.collider = re::physics::Collider{
					.type = re::physics::ColliderType::Sphere,
					.radius = 1.f,
				},
				.mass = 50.0f,
				.friction = 0.0f,
				.gravityFactor = 0.0f,
				.linearDamping = 10.0f,
				.lockRotation = re::Vector3(true),
			});

		auto [solidV, solidI] = re::detail::PrimitiveBuilder::CreateOctahedron(false);
		auto [wireV, wireI] = re::detail::PrimitiveBuilder::CreateOctahedron(true);

		const auto texture = m_manager.Get<re::Texture>("0.png");
		re::Material material;
		material.texture = texture;
		auto [cubeV, cubeI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::White);
		scene
			.CreateEntity()
			.Add<re::MaterialComponent>(material)
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::detail::OpaqueTag>()
			.Add<re::TransformComponent>({ .position = { 3.f, 0.f, -5.f } })
			.Add<re::StaticMeshComponent3D>(cubeV, cubeI);

		constexpr std::size_t gridSize = 1;
		constexpr float spacing = 3.0f;
		constexpr float offset = (gridSize - 1) * spacing * 0.5f;

		auto solidMesh = std::make_shared<re::StaticMesh>(solidV, solidI);
		auto wireframeMesh = std::make_shared<re::StaticMesh>(wireV, wireI);
		for (std::size_t x = 0; x < gridSize; ++x)
		{
			for (std::size_t y = 0; y < gridSize; ++y)
			{
				for (std::size_t z = 0; z < gridSize; ++z)
				{
					const float posX = x * spacing - offset;
					const float posY = y * spacing - offset;
					const float posZ = (z * spacing - offset) - 10.f;

					scene.CreateEntity()
						.Add<re::detail::TransparentTag>()
						.Add<re::Dirty<re::TransformComponent>>()
						.Add<re::TransformComponent>({
							.position = { posX, posY, posZ },
							.scale = { 1.0f, 1.0f, 1.0f },
						})
						.Add<re::StaticMeshComponent3D>(solidMesh, false);

					scene.CreateEntity()
						.Add<re::detail::TransparentTag>()
						.Add<re::Dirty<re::TransformComponent>>()
						.Add<re::TransformComponent>({
							.position = { posX, posY, posZ },
							.scale = { 1.0f, 1.0f, 1.0f },
						})
						.Add<re::StaticMeshComponent3D>(wireframeMesh, true);
				}
			}
		}

		const auto model = m_manager.Get<re::Model>("model/Model.obj");
		if (model)
		{
			for (const auto& [vertices, indices, material] : model->GetParts())
			{
				std::vector<re::Vector3f> positions;
				positions.reserve(vertices.size());
				std::ranges::transform(vertices, std::back_inserter(positions), [](const auto& vertex) {
					return vertex.position;
				});

				m_tempCollisionMeshes.push_back(std::move(positions));

				const auto& safePositions = m_tempCollisionMeshes.back();

				scene.CreateEntity()
					.Add<re::RigidBodyComponent>({
						.type = re::physics::BodyType::Static,
						.collider = re::physics::Collider{
							.type = re::physics::ColliderType::TriangleMesh,
							.vertices = safePositions.data(),
							.vertexCount = safePositions.size(),
							.indices = indices.data(),
							.indexCount = indices.size(),
						},
						.friction = 0.5f,
						.restitution = 0.f,
					})
					.Add<re::Dirty<re::TransformComponent>>()
					.Add<re::TransformComponent>({
						.position = { 0.f, -1.f, -5.f },
						.rotation = { 0.f, 180.f, 0.f },
						.scale = re::Vector3f(1.f),
					})
					.Add<re::detail::OpaqueTag>()
					.Add<re::MaterialComponent>(material)
					.Add<re::StaticMeshComponent3D>(vertices, indices);
			}
		}

		const auto animatedModel = m_manager.Get<re::AnimatedModel>("model/Model.glb");
		if (model)
		{
			for (const auto& [vertices, indices, material] : model->GetParts())
			{
				std::vector<re::Vector3f> positions;
				positions.reserve(vertices.size());
				std::ranges::transform(vertices, std::back_inserter(positions), [](const auto& vertex) {
					return vertex.position;
				});

				m_tempCollisionMeshes.push_back(std::move(positions));

				const auto& safePositions = m_tempCollisionMeshes.back();

				scene.CreateEntity()
					.Add<re::RigidBodyComponent>({
						.type = re::physics::BodyType::Static,
						.collider = re::physics::Collider{
							.type = re::physics::ColliderType::TriangleMesh,
							.vertices = safePositions.data(),
							.vertexCount = safePositions.size(),
							.indices = indices.data(),
							.indexCount = indices.size(),
						},
						.friction = 0.5f,
						.restitution = 0.f,
					})
					.Add<re::Dirty<re::TransformComponent>>()
					.Add<re::TransformComponent>({
						.position = { -3.f, -1.f, -5.f },
						.rotation = { 0.f, 180.f, 0.f },
						.scale = re::Vector3f(1.f),
					})
					.Add<re::detail::OpaqueTag>()
					.Add<re::MaterialComponent>(material)
					.Add<re::StaticMeshComponent3D>(vertices, indices);
			}
		}
	}

	void OnAttach() override
	{
		GetApplication().SetUIOverlayActive(false);
		m_window.SetBackgroundColor(re::Color::White);
	}

	void OnUIDraw() override
	{
		ImGui::Begin("Engine Debug");
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::Text("Delta Time: %.4f sec", ImGui::GetIO().DeltaTime);
		if (ImGui::Button("Click Me!"))
		{
		}
		ImGui::End();
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* keyEvent = event.GetIf<re::Event::KeyPressed>())
		{
			if (keyEvent->key == re::Keyboard::Key::Grave && !keyEvent->alt && !keyEvent->ctrl && !keyEvent->shift)
			{
				GetApplication().SetUIOverlayActive(!GetApplication().IsUIOverlayActive());
			}
		}

		if (GetApplication().IsUIOverlayActive())
		{
			m_firstMouse = true;
			return;
		}

		if (const auto* mouseMoved = event.GetIf<re::Event::MouseMoved>())
		{
			const auto currentX = static_cast<float>(mouseMoved->position.x);
			const auto currentY = static_cast<float>(mouseMoved->position.y);

			if (m_firstMouse)
			{
				m_lastMouseX = currentX;
				m_lastMouseY = currentY;
				m_firstMouse = false;
			}

			const float xOffset = currentX - m_lastMouseX;
			const float yOffset = m_lastMouseY - currentY;

			m_lastMouseX = currentX;
			m_lastMouseY = currentY;

			for (auto&& [entity, transform, camera] : *GetScene().CreateView<re::TransformComponent, re::CameraComponent>())
			{
				if (camera.isPrimal)
				{
					camera.mouseDelta.x += xOffset;
					camera.mouseDelta.y += yOffset;
					break;
				}
			}
		}
	}

private:
	re::ecs::Entity m_solid = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_wireframe = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_manager;

	re::render::IWindow& m_window;

	bool m_firstMouse = true;
	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;

	std::deque<std::vector<re::Vector3f>> m_tempCollisionMeshes;
};

class LauncherApplication final : public re::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
	}

	void OnStart() override
	{
		Window().SetVSyncEnabled(true);

		AddLayout<MenuLayout>(Window());
		// AddLayout<AsteroidsLayout>(Window());
		// AddLayout<MazeLayout>(Window());
		// AddLayout<PianoLayout>(Window());
		// AddLayout<ArcanoidLayout>(Window());

		SwitchLayout<MenuLayout>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		static float timeAccumulator = 0.0f;
		static int frames = 0;

		timeAccumulator += deltaTime;
		frames++;

		if (timeAccumulator >= 1.0f)
		{
			const auto fps = std::format("FPS: {} | ms: {}",
				frames,
				timeAccumulator / static_cast<float>(frames) * 1000.0f);

			// Window().SetTitle(fps);

			frames = 0;
			timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::F1)
			{
				SwitchLayout<MenuLayout>();
			}
			if (e->key == re::Keyboard::Key::F2)
			{
				SwitchLayout<AsteroidsLayout>();
			}
			if (e->key == re::Keyboard::Key::F3)
			{
				SwitchLayout<MazeLayout>();
			}
			if (e->key == re::Keyboard::Key::F4)
			{
				SwitchLayout<PianoLayout>();
			}
			if (e->key == re::Keyboard::Key::F5)
			{
				SwitchLayout<ArcanoidLayout>();
			}
		}
	}

	void OnStop() override
	{
	}
};
