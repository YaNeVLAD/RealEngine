#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include "Lab1/Circle/CircleLayout.hpp"
#include "Lab1/House/HouseLayout.hpp"
#include "Lab1/Letters/LettersLayout.hpp"

struct MenuLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();

		const auto font = m_manager.Get<re::Font>("assets/Roboto.ttf");

		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{}, 0.f, re::Vector2f{ 3.f, 1.f })
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 100, 100 })
			.Add<re::TextComponent>("Hello world!", font, re::Color::White);
	}

private:
	re::AssetManager m_manager;
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
		AddLayout<MenuLayout>();
		AddLayout<LettersLayout>();
		AddLayout<HouseLayout>(Window());
		AddLayout<CircleLayout>(Window());

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
		}

		if (const auto* e = event.GetIf<re::Event::MouseWheelScrolled>())
		{
			for (auto&& [entity, camera] : *CurrentScene().CreateView<re::CameraComponent>())
			{
				const auto newZoom = camera.zoom + e->delta;
				camera.zoom = std::max(newZoom, 0.1f);
			}
		}
	}

	void OnStop() override
	{
	}
};