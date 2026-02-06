#pragma once

#include "Runtime/Components.hpp"

#include <ECS/System/System.hpp>
#include <Runtime/Application.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

struct PlayerTag
{
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
		using namespace re::runtime;

		for (auto&& [id, transform, vel] : *scene.CreateView<TransformComponent, Velocity>())
		{
			transform.position.x += vel.dx * dt;
			transform.position.y += vel.dy * dt;
		}
	}
};

class LauncherApplication final : public re::runtime::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
		using namespace re::runtime;

		CurrentScene().RegisterComponents<Velocity>();

		CurrentScene()
			.AddSystem<MovementSystem>()
			.WithRead<Velocity>()
			.WithWrite<TransformComponent>();

		CurrentScene().BuildSystemGraph();

		CurrentScene()
			.CreateEntity()
			.AddComponent<PlayerTag>()
			.AddComponent<TransformComponent>(re::core::Vector2f{ 0.f, 0.f })
			.AddComponent<Velocity>(10.0f, 5.0f)
			.AddComponent<SpriteComponent>(re::core::Color::Red);
	}

	void OnStart() override
	{
	}

	void OnUpdate(re::core::TimeDelta deltaTime) override
	{
		for (auto&& [entity, transform, velocity] : *CurrentScene().CreateView<re::runtime::TransformComponent, Velocity>())
		{
			// std::cout << "Entity " << entity.Index() << " position: " << transform.position.x << ", " << transform.position.y << std::endl;
		}
	}

	void OnStop() override
	{
	}
};