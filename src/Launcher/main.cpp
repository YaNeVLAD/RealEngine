#include <Runtime/Application.hpp>
#include <Scripting/ScriptLoader.hpp>

#include <iostream>
#include <stdexcept>

struct PlayerData
{
	float speed;
	int health;
};

void LoadGameConfig(re::runtime::Application& engine)
{
	using namespace re::scripting;

	std::cout << "Loading scripts..." << std::endl;

	ScriptLoader loader;
	loader.LoadFromFile("aboba");

	auto& scene = engine.CurrentScene();
	auto player = scene.CreateEntity();

	player.Add<PlayerData>(50.0f, 100);

	const auto& id = player.Get<int>();

	std::cout << id << std::endl;
}

int main()
{
	try
	{
		re::runtime::Application app;

		app.Initialize("Real Engine: Paradox Style", 1280, 720);

		LoadGameConfig(app);

		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}