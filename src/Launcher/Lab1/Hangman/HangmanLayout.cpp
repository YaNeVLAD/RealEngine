#include "HangmanLayout.hpp"

#include "Runtime/Application.hpp"

#include <fstream>
#include <random>

namespace
{

constexpr auto FONT_PATH = "assets/Roboto.ttf";
const re::String RUSSIAN_ALPHABET = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";

void CreateHangman(re::ecs::Scene& scene)
{
	scene.CreateEntity()
		.Add<HangmanPartComponent>(0, re::Color::Brown)
		.Add<re::TransformComponent>(re::Vector2f{ -500.0f, -175.0f })
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 6.0f, 50.0f });

	scene.CreateEntity()
		.Add<HangmanPartComponent>(1)
		.Add<re::TransformComponent>(re::Vector2f{ -500.0f, -125.0f })
		.Add<re::CircleComponent>(re::Color::Transparent, 30.0f);

	scene.CreateEntity()
		.Add<HangmanPartComponent>(2)
		.Add<re::TransformComponent>(re::Vector2f{ -500.0f, -50.0f })
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 8.0f, 100.0f });

	scene.CreateEntity()
		.Add<HangmanPartComponent>(3)
		.Add<re::TransformComponent>(re::Vector2f{ -525.0f, -50.0f }, -45.f)
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 70.7f, 5.0f });

	scene.CreateEntity()
		.Add<HangmanPartComponent>(4)
		.Add<re::TransformComponent>(re::Vector2f{ -475.0f, -50.0f }, 45.f)
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 70.7f, 5.0f });

	scene.CreateEntity()
		.Add<HangmanPartComponent>(5)
		.Add<re::TransformComponent>(re::Vector2f{ -525.0f, 25.0f }, -45.f)
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 70.7f, 5.0f });

	scene.CreateEntity()
		.Add<HangmanPartComponent>(6)
		.Add<re::TransformComponent>(re::Vector2f{ -475.0f, 25.0f }, 45.f)
		.Add<re::RectangleComponent>(re::Color::Transparent, re::Vector2f{ 70.7f, 5.0f });
}

} // namespace

void HangmanLayout::OnCreate()
{
	m_assetManager.Get<re::Font>(FONT_PATH);

	GetScene()
		.AddSystem<CollisionSystem>()
		.WithRead<re::TransformComponent, re::BoxColliderComponent>()
		.WithWrite<re::BoxColliderComponent>();

	GetScene().BuildSystemGraph();

	LoadWords();
	StartNewGame();
}

void HangmanLayout::OnUpdate(const re::core::TimeDelta ts)
{
	const auto& gameState = GetScene().GetComponent<GameStateComponent>(m_gameStateEntity);
	if (gameState.gameState == GameStateComponent::State::Playing)
	{
		UpdateUI();
	}
}

void HangmanLayout::OnEvent(re::Event const& event)
{
	if (const auto* mousePressed = event.GetIf<re::Event::MouseButtonPressed>())
	{
		if (mousePressed->button == re::Mouse::Button::Left)
		{
			const auto& gameState = GetScene().GetComponent<GameStateComponent>(m_gameStateEntity);
			const auto mousePos = m_window.ToWorldPos(mousePressed->position);

			if (gameState.gameState == GameStateComponent::State::Playing)
			{
				for (auto&& [_, letterButton, collider] : *GetScene().CreateView<LetterButtonComponent, re::BoxColliderComponent>())
				{
					if (letterButton.buttonState == LetterButtonComponent::State::Normal && collider.Contains(mousePos))
					{
						CheckGuess(letterButton.letter);
						break;
					}
				}
			}
			else
			{
				for (auto&& [_, button, collider] : *GetScene().CreateView<DialogButtonComponent, re::BoxColliderComponent>())
				{
					if (collider.Contains(mousePos))
					{
						if (button.action == DialogButtonComponent::Action::PlayAgain)
						{
							StartNewGame();
						}
						else if (button.action == DialogButtonComponent::Action::Quit)
						{
							GetApplication().Shutdown();
						}
						break;
					}
				}
			}
		}
	}

	if (const auto* textEntered = event.GetIf<re::Event::TextEntered>())
	{
		const auto ch = re::String::ToUpper(textEntered->symbol);
		if (RUSSIAN_ALPHABET.Find(ch) != re::String::NPos)
		{
			for (auto&& [_, letterButton] : *GetScene().CreateView<LetterButtonComponent>())
			{
				if (letterButton.letter == ch
					&& letterButton.buttonState == LetterButtonComponent::State::Normal)
				{
					CheckGuess(ch);
					break;
				}
			}
		}
	}
}

void HangmanLayout::LoadWords()
{
	std::ifstream file("assets/words.txt");
	std::string line;
	while (std::getline(file, line))
	{
		const auto delimiterPos = line.find(';');
		if (delimiterPos != std::string::npos)
		{
			re::String word(line.substr(0, delimiterPos));
			std::string hint = line.substr(delimiterPos + 1);

			m_words.emplace_back(word.ToUpper(), hint);
		}
	}
}

void HangmanLayout::StartNewGame()
{
	for (auto&& [entity, _] : *GetScene().CreateView<re::TransformComponent>())
	{
		GetScene().DestroyEntity(entity);
	}

	if (GetScene().IsValid(m_gameStateEntity))
	{
		GetScene().DestroyEntity(m_gameStateEntity);
	}
	GetScene().ConfirmChanges();

	m_gameStateEntity = GetScene().CreateEntity().GetEntity();
	GetScene().AddComponent<GameStateComponent>(m_gameStateEntity);
	auto& gameState = GetScene().GetComponent<GameStateComponent>(m_gameStateEntity);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution distrib(0, static_cast<int>(m_words.size()) - 1);
	auto const& [word, hint] = m_words[distrib(gen)];

	gameState.secretWord = word;
	gameState.hint = hint;

	CreateUI();
}

void HangmanLayout::CreateUI()
{
	auto& scene = GetScene();
	auto const& gameState = scene.GetComponent<GameStateComponent>(m_gameStateEntity);
	auto font = m_assetManager.Get<re::Font>(FONT_PATH);

	constexpr float letterSpacing = 60.0f;
	const float startX = -static_cast<float>(gameState.secretWord.Length() - 1) * letterSpacing / 2.0f;
	for (size_t i = 0; i < gameState.secretWord.Length(); ++i)
	{
		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{ startX + static_cast<float>(i) * letterSpacing, 200.0f })
			.Add<re::TextComponent>("_", font, re::Color::White, 64.f)
			.Add<WordLetterComponent>(gameState.secretWord[i], false);
	}

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0, 120.0f })
		.Add<re::TextComponent>(gameState.hint, font, re::Color::White, 24.f);

	constexpr int LETTERS_PER_ROW = 13;
	constexpr float BUTTON_SIZE = 50.0f;
	constexpr float BUTTON_SPACING = 10.0f;
	constexpr float TOTAL_WIDTH = LETTERS_PER_ROW * (BUTTON_SIZE + BUTTON_SPACING) - BUTTON_SPACING;

	for (size_t i = 0; i < RUSSIAN_ALPHABET.Size(); ++i)
	{
		char32_t letter = RUSSIAN_ALPHABET[i];

		const int row = (int)i / LETTERS_PER_ROW;
		const int col = (int)i % LETTERS_PER_ROW;

		const float x = -TOTAL_WIDTH / 2.0f + (float)col * (BUTTON_SIZE + BUTTON_SPACING) + BUTTON_SIZE / 2.0f;
		const float y = -150.0f + (float)row * (BUTTON_SIZE + BUTTON_SPACING);

		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{ x, y })
			.Add<LetterButtonComponent>(letter, LetterButtonComponent::State::Normal)
			.Add<re::RectangleComponent>(re::Color::Magenta, re::Vector2f{ BUTTON_SIZE, BUTTON_SIZE })
			.Add<re::TextComponent>(letter, font, re::Color::White, 32.f)
			.Add<re::BoxColliderComponent>(re::Vector2f{ BUTTON_SIZE, BUTTON_SIZE });
	}

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ -650.0f, 250.0f })
		.Add<re::RectangleComponent>(re::Color::Gray, re::Vector2f{ 200.0f, 5.0f });

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ -650.0f, 25.0f })
		.Add<re::RectangleComponent>(re::Color::Gray, re::Vector2f{ 5.0f, 450.0f });

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ -575.0f, -200.0f })
		.Add<re::RectangleComponent>(re::Color::Gray, re::Vector2f{ 150.0f, 5.0f });

	CreateHangman(scene);
}

void HangmanLayout::UpdateUI()
{
	auto& scene = GetScene();
	const auto& gameState = scene.GetComponent<GameStateComponent>(m_gameStateEntity);

	for (auto&& [entity, wordLetter, text] : *scene.CreateView<WordLetterComponent, re::TextComponent>())
	{
		if (wordLetter.isVisible)
		{
			text.text = wordLetter.letter;
		}
	}

	for (auto&& [entity, button, text] : *scene.CreateView<LetterButtonComponent, re::TextComponent>())
	{
		if (button.buttonState == LetterButtonComponent::State::Correct)
		{
			text.color = re::Color::Green;
		}
		else if (button.buttonState == LetterButtonComponent::State::Incorrect)
		{
			text.color = re::Color::Red;
		}
	}

	for (auto&& [entity, part, rect] : *scene.CreateView<HangmanPartComponent, re::RectangleComponent>())
	{
		if (part.order < gameState.incorrectGuesses)
		{
			rect.color = part.color;
		}
	}

	for (auto&& [entity, part, circle] : *scene.CreateView<HangmanPartComponent, re::CircleComponent>())
	{
		if (part.order < gameState.incorrectGuesses)
		{
			circle.color = part.color;
		}
	}
}

void HangmanLayout::CheckGuess(const char32_t letter)
{
	auto& gameState = GetScene().GetComponent<GameStateComponent>(m_gameStateEntity);
	if (gameState.gameState != GameStateComponent::State::Playing)
	{
		return;
	}

	gameState.guessedLetters.push_back(letter);

	bool found = false;
	for (auto&& [entity, button] : *GetScene().CreateView<LetterButtonComponent>())
	{
		if (button.letter == letter)
		{
			if (gameState.secretWord.Find(letter) != re::String::NPos)
			{
				button.buttonState = LetterButtonComponent::State::Correct;
				found = true;
			}
			else
			{
				button.buttonState = LetterButtonComponent::State::Incorrect;
			}
			break;
		}
	}

	if (found)
	{
		for (auto&& [entity, wordLetter] : *GetScene().CreateView<WordLetterComponent>())
		{
			if (wordLetter.letter == letter)
			{
				wordLetter.isVisible = true;
			}
		}
	}
	else
	{
		gameState.incorrectGuesses++;
	}

	CheckWinLoss();
}

void HangmanLayout::CheckWinLoss()
{
	auto& gameState = GetScene().GetComponent<GameStateComponent>(m_gameStateEntity);

	if (gameState.incorrectGuesses >= 7)
	{
		gameState.gameState = GameStateComponent::State::Lost;
		ShowEndGameDialog(false);
		UpdateUI();
		return;
	}

	bool allVisible = true;
	for (auto&& [entity, wordLetter] : *GetScene().CreateView<WordLetterComponent>())
	{
		if (!wordLetter.isVisible)
		{
			allVisible = false;
			break;
		}
	}

	if (allVisible)
	{
		gameState.gameState = GameStateComponent::State::Won;
		ShowEndGameDialog(true);
		UpdateUI();
	}
}

void HangmanLayout::ShowEndGameDialog(const bool won)
{
	auto& scene = GetScene();
	auto font = m_assetManager.Get<re::Font>(FONT_PATH);

	for (auto&& [entity, _] : *scene.CreateView<LetterButtonComponent>())
	{
		scene.DestroyEntity(entity);
	}

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0, 0 })
		.Add<re::RectangleComponent>(re::Color(0, 0, 0, 150), re::Vector2f{ 1280, 720 })
		.Add<DialogButtonComponent>();

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0, 50.0f })
		.Add<re::TextComponent>(won ? "You Won!" : "You Lost!", font, re::Color::White, 80.f)
		.Add<DialogButtonComponent>();

	if (!won)
	{
		const auto& gameState = scene.GetComponent<GameStateComponent>(m_gameStateEntity);
		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{ 0, 0 })
			.Add<re::TextComponent>("The word was: " + gameState.secretWord, font, re::Color::White, 32.f)
			.Add<DialogButtonComponent>();
	}

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0, -75.0f })
		.Add<re::TextComponent>("Play Again", font, re::Color::Green, 48.f)
		.Add<re::BoxColliderComponent>(re::Vector2f{ 300, 60 })
		.Add<DialogButtonComponent>(DialogButtonComponent::Action::PlayAgain);

	scene.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0, -150.0f })
		.Add<re::TextComponent>("Quit", font, re::Color::Red, 48.f)
		.Add<re::BoxColliderComponent>(re::Vector2f{ 150, 60 })
		.Add<DialogButtonComponent>(DialogButtonComponent::Action::Quit);
}
