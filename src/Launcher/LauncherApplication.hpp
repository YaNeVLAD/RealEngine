#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>

#include <Core/Math/Vector3.hpp>
#include <RVM/VirtualMachine.hpp>

#include "Lab1/Circle/CircleLayout.hpp"
#include "Lab1/Hangman/HangmanLayout.hpp"
#include "Lab1/House/HouseLayout.hpp"
#include "Lab1/Letters/LettersLayout.hpp"

#include "Lab2/Alchemy/AlchemyLayout.hpp"
#include "Lab3/Asteroids/AsteroidsLayout.hpp"

#include <filesystem>
#include <iostream>

#include <IgniLang/AstBuilder.hpp>
#include <IgniLang/BuildCst.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <fsm/cfg.hpp>
#include <fsm/ll1/table_builder.hpp>
#include <fsm/ll1/table_io.hpp>
#include <fsm/string_symbol_generator.hpp>

struct MenuLayout final : re::Layout
{
	MenuLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
		auto lexer = igni::CreateLexer();

		constexpr auto OUT_TABLE_FILE = "assets/out_igni_grammar.txt";

		fsm::ll1::table<std::string> table;
		if (std::filesystem::exists(OUT_TABLE_FILE))
		{
			std::ifstream file(OUT_TABLE_FILE);
			table = fsm::ll1::io::load_from_text<std::string>(file);
		}
		else
		{
			std::ifstream file("assets/igni_grammar.txt");
			auto grammar = fsm::cfg_load(file)
				| fsm::transforms::remove_left_recursion
				| fsm::transforms::left_factor;

			table = fsm::ll1::table_builder<std::string>(grammar, "#", "$")
						.with_collision_strategy(fsm::ll1::collision_strategy::keep_first)
						.build();
			std::ofstream out(OUT_TABLE_FILE);
			fsm::ll1::io::save_to_text(table, out);
		}

		std::string src = "package main; val _a = true && false; val _b = (a+b)<=a*-b; val _c = true && false || (a>b+c)";
		lexer.change_source(src);
		auto cst = igni::BuildCst(table, "Program", lexer);

		cst->Print();

		auto ast = igni::AstBuilder().BuildAnyExpr(cst.get());
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		auto cube = scene
						.CreateEntity()
						.Add<re::TransformComponent3D>(
							{ .position = { 0.f, 0.f, -3.f },
								.rotation = { 45.f, 45.f, 0.f } })
						.Add<re::CubeComponent>(re::Color::Red);

		m_cube = cube.GetEntity();

		using namespace re::rvm;
		VirtualMachine vm;
		vm.RegisterNative("print", [](std::vector<Value> const& args) -> Value {
			std::cout << ">>> [SCRIPT]: ";
			for (const auto& arg : args)
			{
				std::cout << arg << " ";
			}
			std::cout << "\n";

			return Null;
		});

		vm.RegisterNative("read", [](std::vector<Value> const&) -> Value {
			std::string input;
			std::getline(std::cin, input);

			return re::String(input);
		});

		if (const auto chunk = m_manager.Get<Chunk>("scripts/fibb_recursion_test.rbc"))
		{
			std::cout << "scripts/fibb_recursion_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
		if (const auto chunk = m_manager.Get<Chunk>("scripts/function_callback_test.rbc"))
		{
			std::cout << "scripts/function_callback_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
		if (const auto chunk = m_manager.Get<Chunk>("scripts/user_types_test.rbc"))
		{
			std::cout << "scripts/user_types_test.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
		if (const auto chunk = m_manager.Get<Chunk>("scripts/array_tests.rbc"))
		{
			std::cout << "scripts/array_tests.rbc\n";
			std::cout << "==============================\n";
			vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
	}

	void OnUpdate(re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& transform = scene.GetComponent<re::TransformComponent3D>(m_cube);
		transform.rotation.x += 45.0f * dt;
		transform.rotation.y += 45.0f * dt;
	}

private:
	re::ecs::Entity m_cube = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_manager;

	re::render::IWindow& m_window;
};

class LauncherApplication final : public re::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
	}

	void OnStart() override
	{
		AddLayout<MenuLayout>(Window());
		AddLayout<LettersLayout>();
		AddLayout<HouseLayout>(Window());
		AddLayout<CircleLayout>(Window());
		AddLayout<HangmanLayout>(Window());
		AddLayout<AlchemyLayout>(Window());
		AddLayout<AsteroidsLayout>(Window());

		SwitchLayout<MenuLayout>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		static float timeAccumulator = 0.0f;
		static int frames = 0;

		timeAccumulator += deltaTime;
		frames++;

		if (timeAccumulator >= 1.0f)
		{
			const auto fps = std::format("FPS: {} | ms: {}",
				frames,
				timeAccumulator / static_cast<float>(frames) * 1000.0f);

			Window().SetTitle(fps);

			frames = 0;
			timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::Num1)
			{
				SwitchLayout<MenuLayout>();
			}
			if (e->key == re::Keyboard::Key::Num2)
			{
				SwitchLayout<LettersLayout>();
			}
			if (e->key == re::Keyboard::Key::Num3)
			{
				SwitchLayout<HouseLayout>();
			}
			if (e->key == re::Keyboard::Key::Num4)
			{
				SwitchLayout<CircleLayout>();
			}
			if (e->key == re::Keyboard::Key::Num5)
			{
				SwitchLayout<HangmanLayout>();
			}
			if (e->key == re::Keyboard::Key::Num6)
			{
				SwitchLayout<AlchemyLayout>();
			}
			if (e->key == re::Keyboard::Key::Num7)
			{
				SwitchLayout<AsteroidsLayout>();
			}
		}

		if (const auto* e = event.GetIf<re::Event::MouseWheelScrolled>())
		{
			for (auto&& [entity, camera] : *CurrentScene().CreateView<re::CameraComponent>())
			{
				const auto newZoom = camera.zoom + e->delta * 0.1f;
				camera.zoom = std::max(newZoom, 0.1f);
			}
		}
	}

	void OnStop() override
	{
	}
};