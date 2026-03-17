#pragma once

#include <Core/Math/Vector3.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include "Lab3/Asteroids/AsteroidsLayout.hpp"

#include "CameraControlSystem.hpp"

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

		auto [solidV, solidI] = re::detail::PrimitiveBuilder::CreateOctahedron(false);
		auto [wireV, wireI] = re::detail::PrimitiveBuilder::CreateOctahedron(true);

		const auto solid = scene.CreateEntity()
							   .Add<re::TransformComponent>(
								   { .position = { 0.f, 0.f, -4.f },
									   .rotation = { 0.f, 0.f, 0.f },
									   .scale = { 1.5f, 1.5f, 1.5f } })
							   .Add<re::MeshComponent3D>(solidV, solidI, false);

		const auto wireframe = scene.CreateEntity()
								   .Add<re::TransformComponent>(
									   { .position = { 0.f, 0.f, -4.f },
										   .rotation = { 0.f, 0.f, 0.f },
										   .scale = { 1.5f, 1.5f, 1.5f } })
								   .Add<re::MeshComponent3D>(wireV, wireI, true);

		m_solid = solid.GetEntity();
		m_wireframe = wireframe.GetEntity();
	}

	void OnAttach() override
	{
		m_window.SetCursorLocked(true);
		m_window.SetBackgroundColor(re::Color::White);
	}

	void OnDetach() override
	{
		m_window.SetCursorLocked(false);
		m_window.SetBackgroundColor(re::Color::Black);
	}

	void OnEvent(re::Event const& event) override
	{
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
		AddLayout<MenuLayout>(Window());
		AddLayout<AsteroidsLayout>(Window());

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

			Window().SetTitle(fps);

			frames = 0;
			timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::Num1)
			{
				SwitchLayout<MenuLayout>();
			}
			if (e->key == re::Keyboard::Key::Num2)
			{
				SwitchLayout<AsteroidsLayout>();
			}
		}

		if (const auto* e = event.GetIf<re::Event::MouseWheelScrolled>())
		{
			for (auto&& [entity, camera] : *CurrentScene().CreateView<re::CameraComponent>())
			{
				const auto newZoom = camera.zoom + e->delta * 0.1f;
				camera.zoom = std::max(newZoom, 0.1f);
			}
		}
	}

	void OnStop() override
	{
	}
};
