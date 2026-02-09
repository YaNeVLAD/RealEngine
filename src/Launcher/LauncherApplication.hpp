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
	}

	void OnStop() override
	{
	}
};