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
#include "RVM/Assembler.hpp"

#include <filesystem>
#include <iostream>

#include <IgniLang/AST/AstConverter.hpp>
#include <IgniLang/CST/CstBuilder.hpp>
#include <IgniLang/Compiler/TextCompiler.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <fsm/cfg.hpp>
#include <fsm/slr/parser.hpp>
#include <fsm/slr/table.hpp>
#include <fsm/slr/table_builder.hpp>
#include <fsm/string_symbol_generator.hpp>

struct CompilerLayout final : re::Layout
{
	explicit CompilerLayout(re::Application& app)
		: Layout(app)
	{
		using namespace re::rvm;
		m_vm.RegisterNative("print", [](std::vector<Value> const& args) -> Value {
			std::cout << ">>> [SCRIPT]: ";
			for (const auto& arg : args)
			{
				std::cout << arg << " ";
			}
			std::cout << "\n";

			return Null;
		});

		m_vm.RegisterNative("read", [](std::vector<Value> const&) -> Value {
			std::string input;
			std::getline(std::cin, input);

			return re::String(input);
		});

		RunRVMTest("scripts/function_test.rbc");

		RunRVMTest("scripts/fibb_recursion_test.rbc");

		RunRVMTest("scripts/function_callback_test.rbc");

		RunRVMTest("scripts/user_types_test.rbc");

		RunRVMTest("scripts/array_tests.rbc");

		RunRVMTest("scripts/closure_tests.rbc");
	}

	void OnCreate() override
	{
		std::ifstream file("assets/igni_grammar.txt");
		auto grammar = fsm::cfg_load<re::String>(file);

		fsm::slr::table_builder builder(grammar);
		auto table = builder
						 .with_epsilon("<EPSILON>")
						 .with_end_marker("<EOF>")
						 .build(fsm::slr::collision_policy::prefer_shift);

		std::string src = R"(
			package main;

			fun main() {
				print(!true);
				print(!false);
				val y = 10;
				var x = ++y;
				print("y:", y, "x:", x);
			}
		)";
		auto tokens = igni::CreateLexer(src).tokenize();

		fsm::slr::parser parser(table, "<EPSILON>");
		auto cstRoot = igni::CstBuilder(parser, "<EPSILON>").Build(tokens);

		auto astRoot = igni::AstConverter().Convert(cstRoot.get());
		astRoot->Print();

		igni::compiler::TextCompiler textCompiler;
		auto asmCode = textCompiler.Compile(astRoot.get());

		std::cout << asmCode << std::endl;

		re::rvm::Chunk chunk;
		if (re::rvm::Assembler assembler; assembler.Compile(asmCode, chunk))
		{
			m_vm.Interpret(chunk);
		}
	}

private:
	re::AssetManager m_manager;
	re::rvm::VirtualMachine m_vm;

	void RunRVMTest(const char* file)
	{
		if (const auto chunk = m_manager.Get<re::rvm::Chunk>(file))
		{
			std::cout << file << "\n";
			std::cout << "==============================\n";
			m_vm.Interpret(*chunk);
			std::cout << "==============================" << std::endl;
		}
	}
};

struct MenuLayout final : re::Layout
{
	MenuLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		const auto cube = scene
							  .CreateEntity()
							  .Add<re::TransformComponent3D>(
								  { .position = { 0.f, 0.f, -3.f },
									  .rotation = { 45.f, 45.f, 0.f } })
							  .Add<re::CubeComponent>(re::Color::Red);

		m_cube = cube.GetEntity();
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
		auto& scene = GetScene();

		auto& transform = scene.GetComponent<re::TransformComponent3D>(m_cube);
		transform.rotation.x += 45.0f * dt;
		transform.rotation.y += 45.0f * dt;
	}

private:
	re::rvm::VirtualMachine vm;

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
		AddLayout<CompilerLayout>();
		AddLayout<MenuLayout>(Window());
		AddLayout<LettersLayout>();
		AddLayout<HouseLayout>(Window());
		AddLayout<CircleLayout>(Window());
		AddLayout<HangmanLayout>(Window());
		AddLayout<AlchemyLayout>(Window());
		AddLayout<AsteroidsLayout>(Window());

		SwitchLayout<CompilerLayout>();
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