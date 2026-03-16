#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include <Core/Math/Vector3.hpp>
#include <RVM/VirtualMachine.hpp>

#include "Lab1/Circle/CircleLayout.hpp"
#include "Lab1/Hangman/HangmanLayout.hpp"
#include "Lab1/House/HouseLayout.hpp"
#include "Lab1/Letters/LettersLayout.hpp"

#include "Lab2/Alchemy/AlchemyLayout.hpp"
#include "Lab3/Asteroids/AsteroidsLayout.hpp"

#include <iostream>

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

		auto cube = scene
						.CreateEntity()
						.Add<re::TransformComponent3D>(
							{ .position = { 0.f, 0.f, -3.f },
								.rotation = { 45.f, 45.f, 0.f } })
						.Add<re::CubeComponent>(re::Color::Red);

		m_cube = cube.GetEntity();

		using namespace re::rvm;
		VirtualMachine vm;
		vm.RegisterNative("print", [](std::vector<Value> const& args) -> Value {
			std::cout << ">>> [SCRIPT]: ";
			for (const auto& arg : args)
			{
				std::cout << arg << " ";
			}
			std::cout << "\n";

			return Null;
		});

		vm.RegisterNative("read", [](std::vector<Value> const&) -> Value {
			std::string input;
			std::getline(std::cin, input);

			return re::String(input);
		});

		if (const auto chunk = m_manager.Get<Chunk>("scripts/fibb_recursion_test.rbc"))
		{
			std::cout << "scripts/fibb_recursion_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
		if (const auto chunk = m_manager.Get<Chunk>("scripts/function_callback_test.rbc"))
		{
			std::cout << "scripts/function_callback_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
		if (const auto chunk = m_manager.Get<Chunk>("scripts/user_types_test.rbc"))
		{
			std::cout << "scripts/user_types_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
	}

	void OnUpdate(re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& transform = scene.GetComponent<re::TransformComponent3D>(m_cube);
		transform.rotation.x += 45.0f * dt;
		transform.rotation.y += 45.0f * dt;
	}

private:
	re::ecs::Entity m_cube = re::ecs::Entity::INVALID_ID;
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
		AddLayout<LettersLayout>();
		AddLayout<HouseLayout>(Window());
		AddLayout<CircleLayout>(Window());
		AddLayout<HangmanLayout>(Window());
		AddLayout<AlchemyLayout>(Window());
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
				SwitchLayout<LettersLayout>();
			}
			if (e->key == re::Keyboard::Key::Num3)
			{
				SwitchLayout<HouseLayout>();
			}
			if (e->key == re::Keyboard::Key::Num4)
			{
				SwitchLayout<CircleLayout>();
			}
			if (e->key == re::Keyboard::Key::Num5)
			{
				SwitchLayout<HangmanLayout>();
			}
			if (e->key == re::Keyboard::Key::Num6)
			{
				SwitchLayout<AlchemyLayout>();
			}
			if (e->key == re::Keyboard::Key::Num7)
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