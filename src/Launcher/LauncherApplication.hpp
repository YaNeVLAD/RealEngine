#pragma once

#include <Core/Math/Vector3.hpp>
#include <RVM/EventLoop.hpp>
#include <RVM/VirtualMachine.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include "Lab1/Circle/CircleLayout.hpp"
#include "Lab1/Hangman/HangmanLayout.hpp"
#include "Lab1/House/HouseLayout.hpp"
#include "Lab1/Letters/LettersLayout.hpp"

#include "Lab2/Alchemy/AlchemyLayout.hpp"
#include "Lab3/Asteroids/AsteroidsLayout.hpp"

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

		const auto cube = scene
							  .CreateEntity()
							  .Add<re::TransformComponent3D>(
								  { .position = { 0.f, 0.f, -3.f },
									  .rotation = { 45.f, 45.f, 0.f } })
							  .Add<re::CubeComponent>(re::Color::Red);

		m_cube = cube.GetEntity();

		using namespace re::rvm;
		EventLoop eventLoop;

		vm.RegisterNative("println", [](const std::vector<Value>& args) -> Value {
			if (!args.empty() && std::holds_alternative<ArrayInstancePtr>(args[0]))
			{
				for (const auto arr = std::get<ArrayInstancePtr>(args[0]); const auto& arg : arr->elements)
				{
					std::cout << arg;
				}
			}
			else
			{
				for (const auto& arg : args)
				{
					std::cout << arg;
				}
			}
			std::cout << std::endl;

			return Null;
		});

		vm.RegisterNative("delay", [&](const std::vector<Value>& args) -> Value {
			if (args.empty() || !std::holds_alternative<Int>(args[0]))
			{
				return Null;
			}

			const std::uint64_t ms = std::get<Int>(args[0]);

			vm.RequestDelay(ms);

			return Null;
		});

		vm.SetDelayHandler([&eventLoop](const CoroutinePtr& coro, const std::uint64_t ms) {
			eventLoop.Delay(coro, ms);
		});

		if (const auto script = m_manager.Get<Chunk>("scripts/coroutine_async_tests.rbc"))
		{
			vm.Interpret(*script);
			eventLoop.Run(vm);
		}
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& transform = scene.GetComponent<re::TransformComponent3D>(m_cube);
		transform.rotation.x += 45.0f * dt;
		transform.rotation.y += 45.0f * dt;
	}

private:
	re::rvm::VirtualMachine vm;

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