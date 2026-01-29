#pragma once

#include "Runtime/System.hpp"

#include <Runtime/Application.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

struct PlayerTag
{
};

struct Transform
{
	float x, y;
};

struct Velocity
{
	float dx, dy;
};

class MovementSystem final : public re::runtime::System
{
public:
	void OnUpdate(re::runtime::Scene& scene, re::core::TimeDelta dt) override
	{
		const auto view = scene.View<Transform, Velocity>();

		view.each([dt](auto& transform, auto& vel) {
			transform.x += vel.dx * dt;
			transform.y += vel.dy * dt;
		});
	}
};

class LauncherApplication final : public re::runtime::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
		CurrentScene().AddSystem<MovementSystem>();

		auto player = CurrentScene().CreateEntity();

		player
			.Add<PlayerTag>()
			.Add<Transform>(0.0f, 0.0f)
			.Add<Velocity>(10.0f, 5.0f);
	}

	void OnStart() override
	{
	}

	void OnUpdate(re::core::TimeDelta deltaTime) override
	{
		const auto player = CurrentScene().View<PlayerTag, Transform, Velocity>();

		player.each([](auto& transform, auto& vel) {
			std::cout << "Player position: " << transform.x << ", " << transform.y << std::endl;
			std::cout << "Player velocity: " << vel.dx << ", " << vel.dy << std::endl;
		});
	}

	void OnStop() override
	{
	}
};