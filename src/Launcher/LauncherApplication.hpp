#pragma once

#include <Core/Math/Vector3.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include "Lab3/Asteroids/AsteroidsLayout.hpp"
#include "Runtime/Internal/PrimitiveBuilder.hpp"

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

		auto [solidV, solidI] = re::detail::PrimitiveBuilder::CreateOctahedron(re::Color(255, 255, 255, 128));
		auto [wireV, wireI] = re::detail::PrimitiveBuilder::CreateOctahedron(re::Color::Red);

		const auto solid = scene.CreateEntity()
							   .Add<re::TransformComponent>(
								   { .position = { 0.f, 0.f, -5.f },
									   .rotation = { 30.f, 45.f, 0.f },
									   .scale = { 1.f, 1.f, 1.f } })
							   .Add<re::MeshComponent3D>(solidV, solidI);

		const auto wireframe = scene.CreateEntity()
								   .Add<re::TransformComponent>(
									   { .position = { 0.f, 0.f, -5.f },
										   .rotation = { 30.f, 45.f, 0.f },
										   .scale = { 1.f, 1.f, 1.f } })
								   .Add<re::MeshComponent3D>(wireV, wireI, true);

		m_solid = solid.GetEntity();
		m_wireframe = wireframe.GetEntity();
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& solidT = scene.GetComponent<re::TransformComponent>(m_solid);
		auto& wireframeT = scene.GetComponent<re::TransformComponent>(m_wireframe);

		solidT.rotation.x += 25.f * dt;
		solidT.rotation.y += 25.f * dt;

		wireframeT.rotation.x += 25.f * dt;
		wireframeT.rotation.y += 25.f * dt;
	}

private:
	re::ecs::Entity m_solid = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_wireframe = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_manager;

	re::render::IWindow& m_window;
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
