#pragma once

#include <Runtime/Application.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

struct PlayerData
{
	float speed;
	int health;
};

class LauncherApplication final : public re::runtime::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
		using namespace re::scripting;

		std::cout << "Loading scripts..." << std::endl;

		ScriptLoader loader;
		loader.LoadFromFile("aboba");

		auto& scene = CurrentScene();
		auto player = scene.CreateEntity();

		player.Add<PlayerData>(50.0f, 100);

		const auto& id = player.Get<int>();

		std::cout << id << std::endl;
	}

	void OnStart() override
	{
	}

	void OnUpdate(re::core::TimeDelta deltaTime) override
	{
	}

	void OnStop() override
	{
	}
};