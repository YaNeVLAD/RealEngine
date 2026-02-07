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
		using namespace re::runtime;

		CurrentScene()
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>();

		CurrentScene()
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>();

		CurrentScene().BuildSystemGraph();

		CreateLettersScene(CurrentScene());
	}

	void OnStart() override
	{
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