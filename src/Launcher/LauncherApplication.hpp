#pragma once

#include <ECS/System/System.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

#include "Components.hpp"
#include "Letters.hpp"

class JumpPhysicsSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		constexpr float GRAVITY = 2000.0f;
		constexpr float JUMP_FORCE = -900.0f;

		for (auto&& [_, transform, physics] : *scene.CreateView<re::TransformComponent, GravityComponent>())
		{
			if (physics.startY == 0.0f)
			{
				physics.startY = transform.position.y;
			}

			if (physics.phaseTime > 0.0f)
			{
				physics.phaseTime -= dt;
				return;
			}

			physics.velocity.y += GRAVITY * dt;
			transform.position.y += physics.velocity.y * dt;

			if (transform.position.y >= physics.startY)
			{
				transform.position.y = physics.startY;
				physics.velocity.y = JUMP_FORCE;
			}
		};
	}
};

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

		CurrentScene().BuildSystemGraph();

		CreateKLetter(CurrentScene());
		CreateB1Letter(CurrentScene());
		CreateB2Letter(CurrentScene());
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