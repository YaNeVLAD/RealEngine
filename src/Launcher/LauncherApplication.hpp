#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include <Core/Math/Vector3.hpp>

// #include "Lab1/Circle/CircleLayout.hpp"
// #include "Lab1/Hangman/HangmanLayout.hpp"
// #include "Lab1/House/HouseLayout.hpp"
// #include "Lab1/Letters/LettersLayout.hpp"

// #include "Lab2/Alchemy/AlchemyLayout.hpp"

// #include "Lab3/Asteroids/AsteroidsLayout.hpp"

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
		m_window.SetVSyncEnabled(false);
		auto entity = scene.CreateEntity();
		entity.Add<re::TransformComponent>(
			{ .position = { 0.f, 0.f, -5.f },
				.rotation = { 30.f, 45.f, 0.f },
				.scale = { 1.f, 1.f, 1.f } });

		auto mesh = re::MeshComponent3D{};

		using namespace re;

		// 4 вершины основания (квадрат) и 1 вершина макушки
		mesh.vertices = {
			// Основание (y = -0.5)
			{ Vector3f{ -0.5f, -0.5f, 0.5f }, Vector3f{ 0, -1, 0 }, Color::Red, { 0, 0 } }, // 0
			{ Vector3f{ 0.5f, -0.5f, 0.5f }, Vector3f{ 0, -1, 0 }, Color::Red, { 1, 0 } }, // 1
			{ Vector3f{ 0.5f, -0.5f, -0.5f }, Vector3f{ 0, -1, 0 }, Color::Red, { 1, 1 } }, // 2
			{ Vector3f{ -0.5f, -0.5f, -0.5f }, Vector3f{ 0, -1, 0 }, Color::Red, { 0, 1 } }, // 3
			// Макушка (y = 0.5)
			{ Vector3f{ 0.0f, 0.5f, 0.0f }, Vector3f{ 0, 1, 0 }, Color::Red, { 0.5, 0.5 } } // 4
		};

		// Соединяем индексы (треугольники)
		mesh.indices = {
			// Основание (2 треугольника)
			0, 1, 2,
			2, 3, 0,
			// Боковые грани (4 треугольника)
			0, 1, 4,
			1, 2, 4,
			2, 3, 4,
			3, 0, 4
		};
		entity.Add<MeshComponent3D>(mesh);
		m_piramid = entity.GetEntity();
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& transform = scene.GetComponent<re::TransformComponent>(m_piramid);
		transform.rotation.x += 45.0f * dt;
		transform.rotation.y += 45.0f * dt;
	}

private:
	re::ecs::Entity m_piramid = re::ecs::Entity::INVALID_ID;
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
		// AddLayout<LettersLayout>();
		// AddLayout<HouseLayout>(Window());
		// AddLayout<CircleLayout>(Window());
		// AddLayout<HangmanLayout>(Window());
		// AddLayout<AlchemyLayout>(Window());
		// AddLayout<AsteroidsLayout>(Window());

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
			// if (e->key == re::Keyboard::Key::Num2)
			// {
			// 	SwitchLayout<LettersLayout>();
			// }
			// if (e->key == re::Keyboard::Key::Num3)
			// {
			// 	SwitchLayout<HouseLayout>();
			// }
			// if (e->key == re::Keyboard::Key::Num4)
			// {
			// 	SwitchLayout<CircleLayout>();
			// }
			// if (e->key == re::Keyboard::Key::Num5)
			// {
			// 	SwitchLayout<HangmanLayout>();
			// }
			// if (e->key == re::Keyboard::Key::Num6)
			// {
			// 	SwitchLayout<AlchemyLayout>();
			// }
			// if (e->key == re::Keyboard::Key::Num7)
			// {
			// 	SwitchLayout<AsteroidsLayout>();
			// }
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
