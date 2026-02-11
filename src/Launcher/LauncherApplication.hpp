#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>

#include <iostream>

#include "Components.hpp"
#include "HierarchySystem.hpp"
#include "JumpPhysicsSystem.hpp"
#include "Letters.hpp"

struct LettersLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>();

		scene
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>();

		scene.BuildSystemGraph();
		CreateLettersScene(scene);
	}
};

struct MenuLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{}, 0.f, re::Vector2f{ 3.f, 1.f })
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 100, 100 });
	}
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
		AddLayout<LettersLayout>();
		AddLayout<MenuLayout>();

		SwitchLayout<MenuLayout>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		std::cout << 1.f / deltaTime << std::endl;
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::Enter)
			{
				SwitchLayout<LettersLayout>();
			}
			if (e->key == re::Keyboard::Key::Escape)
			{
				SwitchLayout<MenuLayout>();
			}
		}
	}

	void OnStop() override
	{
	}
};