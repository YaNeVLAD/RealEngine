#pragma once

#include <ECS/System/System.hpp>
#include <Runtime/Application.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

struct Transform
{
	float x, y;
};

struct Velocity
{
	float dx, dy;
};

class MovementSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		for (auto&& [id, transform, vel] : *scene.CreateView<Transform, Velocity>())
		{
			transform.x += vel.dx * dt;
			transform.y += vel.dy * dt;
		}
	}
};

class LauncherApplication final : public re::runtime::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
		CurrentScene().RegisterComponents<Transform, Velocity>();

		CurrentScene()
			.RegisterSystem<MovementSystem>()
			.WithRead<Velocity>()
			.WithWrite<Transform>();

		CurrentScene().BuildSystemGraph();

		const auto player = CurrentScene().CreateEntity();

		CurrentScene().AddComponent<Transform>(player, 0.0f, 0.0f);
		CurrentScene().AddComponent<Velocity>(player, 10.0f, 5.0f);
	}

	void OnStart() override
	{
	}

	void OnUpdate(re::core::TimeDelta deltaTime) override
	{
		for (auto&& [id, transform, velocity] : *CurrentScene().CreateView<Transform, Velocity>())
		{
			std::cout << "Player position: " << transform.x << ", " << transform.y << std::endl;
		}
	}

	void OnStop() override
	{
	}
};