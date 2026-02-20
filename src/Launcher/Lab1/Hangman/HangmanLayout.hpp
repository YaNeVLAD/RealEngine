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

struct GameStateComponent
{
	re::String secretWord;
	re::String hint;
	std::vector<char32_t> guessedLetters;
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
	char32_t letter{};
	bool isVisible = false;
};

struct LetterButtonComponent
{
	char32_t letter{};
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
	int order{};
	re::Color color = re::Color::White;
};

struct DialogButtonComponent
{
	enum class Action
	{
		PlayAgain,
		Quit
	} action;
};

// Повыносить логику, сделать независимую от UI
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
	void CheckGuess(char32_t letter);
	void UpdateUI();
	void CheckWinLoss();
	void ShowEndGameDialog(bool won);

private:
	re::render::IWindow& m_window;

	std::vector<std::pair<re::String, re::String>> m_words;
	re::ecs::Entity m_gameStateEntity = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_assetManager;
};