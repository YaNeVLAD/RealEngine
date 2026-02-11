#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>

#include <iostream>

#include "Components.hpp"
#include "HierarchySystem.hpp"
#include "JumpPhysicsSystem.hpp"
#include "Letters.hpp"

struct LettersTag
{
};

struct MenuTag
{
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
		auto& letterScene = CreateScene<LettersTag>();
		letterScene
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>()
			.RunOnMainThread();

		letterScene
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>()
			.RunOnMainThread();

		letterScene.BuildSystemGraph();
		CreateLettersScene(letterScene);

		auto& menuScene = CreateScene<MenuTag>();
		menuScene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{}, 0.f, re::Vector2f{ 3.f, 1.f })
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 100, 100 });

		ChangeScene<MenuTag>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		std::cout << 1.f / deltaTime << std::endl;
	}

	void OnEvent(re::Event const& event) override
	{
		if (event.Is<re::Event::Resized>())
		{
			std::cout << "Resized from user app" << std::endl;
		}
		if (const auto* e = event.GetIf<re::Event::MouseButtonPressed>())
		{
			std::cout << "Mouse button pressed" << std::endl;
			std::cout << "At " << e->position.x << ", " << e->position.y << std::endl;
		}
		if (const auto* e = event.GetIf<re::Event::MouseButtonReleased>())
		{
			std::cout << "Mouse button released" << std::endl;
			std::cout << "At " << e->position.x << ", " << e->position.y << std::endl;
		}
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::Enter)
			{
				ChangeScene<LettersTag>();
			}
			if (e->key == re::Keyboard::Key::Escape)
			{
				ChangeScene<MenuTag>();
			}
		}
		if (const auto* e = event.GetIf<re::Event::KeyReleased>())
		{
			std::cout << "Key " << e->key << " released" << std::endl;
		}
		if (const auto* e = event.GetIf<re::Event::MouseMoved>())
		{
			std::cout << "MouseMoved to " << e->position.x << ", " << e->position.y << std::endl;
		}
	}

	void OnStop() override
	{
	}
};