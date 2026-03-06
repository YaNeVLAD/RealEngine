#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include <RVM/Assembler.hpp>
#include <RVM/VirtualMachine.hpp>

#include "Lab1/Circle/CircleLayout.hpp"
#include "Lab1/Hangman/HangmanLayout.hpp"
#include "Lab1/House/HouseLayout.hpp"
#include "Lab1/Letters/LettersLayout.hpp"

#include "Lab2/Alchemy/AlchemyLayout.hpp"

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
		auto triangle = scene.CreateEntity();
		triangle.Add<re::TransformComponent>(re::Vector2f{ 0, 0 });

		auto type = re::PrimitiveType::Triangles;
		std::vector<re::Vertex> vertices = {
			{ { 0.f, -150.f }, re::Color::Red },
			{ { 150.f, 150.f }, re::Color::Green },
			{ { -150.f, 150.f }, re::Color::Blue }
		};
		triangle.Add<re::MeshComponent>(vertices, type);

		using namespace re::rvm;

		// x = 10
		// y = 20
		// result = (x + y) * 2
		// return result
		const std::string src = R"(
        CONST 10
        SET x

        CONST 20
        SET y

        GET x
        GET y
        ADD

        CONST 2
        MUL

        SET result

        GET result
        RETURN
		)";

		Assembler assembler;
		if (Chunk chunk; assembler.Compile(src, chunk))
		{
			VirtualMachine vm;
			vm.Interpret(chunk);
		}
	}

	void OnEvent(re::Event const& event) override
	{
	}

private:
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