#pragma once

#include <Core/String.hpp>
#include <RenderCore/IWindow.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

#include <string>
#include <utility>
#include <vector>

#include "CollisionSystem.hpp"

constexpr auto FONT_PATH = "assets/Roboto.ttf";

struct GameStateComponent
{
	std::string secretWord;
	std::string hint;
	std::vector<char> guessedLetters;
	int incorrectGuesses = 0;
	enum class State
	{
		Playing,
		Won,
		Lost
	} gameState
		= State::Playing;
};

struct WordLetterComponent
{
	char letter{};
	bool isVisible = false;
};

struct LetterButtonComponent
{
	char letter{};
	enum class State
	{
		Normal,
		Correct,
		Incorrect
	} buttonState
		= State::Normal;
};

struct HangmanPartComponent
{
	int order;
};

enum class DialogButtonAction
{
	PlayAgain,
	Quit
};

struct DialogButtonComponent
{
	DialogButtonAction action;
};

class HangmanLayout final : public re::Layout
{
public:
	HangmanLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override;
	void OnUpdate(re::core::TimeDelta ts) override;
	void OnEvent(re::Event const& event) override;

private:
	void StartNewGame();
	void LoadWords();
	void CreateUI();
	void CheckGuess(char letter);
	void UpdateUI();
	void CheckWinLoss();
	void ShowEndGameDialog(bool won);

private:
	re::render::IWindow& m_window;

	std::vector<std::pair<re::String, re::String>> m_words;
	re::ecs::Entity m_gameStateEntity = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_assetManager;
};