#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>

#include <iostream>

#include "Components.hpp"
#include "HierarchySystem.hpp"
#include "JumpPhysicsSystem.hpp"
#include "Letters.hpp"

class LauncherApplication final : public re::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
	}

	void OnStart() override
	{
		CurrentScene()
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>()
			.RunOnMainThread();

		CurrentScene()
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>()
			.RunOnMainThread();

		CurrentScene().BuildSystemGraph();

		CreateLettersScene(CurrentScene());
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
			std::cout << "Key " << e->key << " pressed" << std::endl;
		}
		if (const auto* e = event.GetIf<re::Event::KeyReleased>())
		{
			std::cout << "Key " << e->key << " released" << std::endl;
		}
		// if (const auto* e = event.GetIf<re::Event::MouseMoved>())
		// {
		// 	std::cout << "MouseMoved to " << e->position.x << ", " << e->position.y << std::endl;
		// }
	}

	void OnStop() override
	{
	}
};