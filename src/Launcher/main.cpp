#include <Runtime/Application.hpp>
#include <Scripting/ScriptNode.hpp>

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

	// Имитация загрузки скрипта
	// В реальности: ScriptLoader loader; auto root = loader.LoadFromFile("game.txt");
	std::cout << "Loading Paradox-style scripts..." << std::endl;

	// Создаем энтити на основе данных
	auto& registry = engine.CurrentScene();
	// const auto player = registry.CreateEntity();
	//
	// registry.emplace<PlayerData>(player, 50.0f, 100);
}

int main()
{
	try
	{
		re::runtime::Application app;

		// 1. Инициализация систем
		app.Initialize("Saturn Engine: Paradox Style", 1280, 720);

		// 2. Загрузка "скриптовой" части (Game Logic)
		LoadGameConfig(app);

		// 3. Запуск цикла
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}