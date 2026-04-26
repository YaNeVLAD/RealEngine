#include "AsteroidsLayout.hpp"

#include <Runtime/Application.hpp>
#include <Runtime/System/HierarchySystem.hpp>

#include <ctime>

#include "AsteroidCollisionSystem.hpp"
#include "LifetimeSystem.hpp"
#include "MovementSystem.hpp"
#include "ShipSystem.hpp"

namespace
{
constexpr auto FONT_NAME = "Roboto.ttf";
}

using namespace asteroids;

AsteroidsLayout::AsteroidsLayout(re::Application& app, re::render::IWindow& window)
	: Layout(app)
	, m_window(window)
{
	std::srand(static_cast<unsigned>(std::time(nullptr)));
}

void AsteroidsLayout::OnCreate()
{
	m_assetManager.Get<re::Font>(FONT_NAME);

	auto spawnAst = [this](int size, re::Vector2f pos, re::Vector2f vel) {
		SpawnAsteroid(size, pos, vel);
	};
	auto spawnPart = [this](re::Vector2f pos, re::Vector2f vel) {
		SpawnParticle(pos, vel);
	};

	GetScene()
		.AddSystem<ShipControlSystem>()
		.WithRead<re::TransformComponent, ShipComponent>()
		.WithWrite<VelocityComponent>()
		.RunOnMainThread();

	GetScene()
		.AddSystem<MovementSystem>(m_window)
		.WithRead<VelocityComponent, ScreenWrapComponent>()
		.WithWrite<re::TransformComponent>();

	GetScene()
		.AddSystem<AsteroidCollisionSystem>(spawnAst, spawnPart)
		.WithRead<AsteroidComponent, BulletComponent>()
		.WithWrite<ShipComponent>()
		.RunOnMainThread();

	GetScene()
		.AddSystem<LifetimeSystem>()
		.WithRead<BulletComponent, ParticleComponent>()
		.RunOnMainThread();

	GetScene().BuildSystemGraph();
	StartGame();
}

void AsteroidsLayout::StartGame()
{
	for (auto&& [entity, _] : *GetScene().CreateView<re::TransformComponent>())
	{
		if (entity != m_gameStateEntity && entity != m_scoreText && entity != m_livesText)
		{
			GetScene().DestroyEntity(entity);
		}
	}

	GetScene().ConfirmChanges();

	if (!GetScene().IsValid(m_gameStateEntity))
	{
		m_gameStateEntity = GetScene().CreateEntity().GetEntity();
		GetScene().AddComponent<AsteroidGameStateComponent>(m_gameStateEntity);
		CreateUI();
	}
	else
	{
		auto& state = GetScene().GetComponent<AsteroidGameStateComponent>(m_gameStateEntity);
		state.score = 0;
		state.lives = 3;
		state.isGameOver = false;
	}

	m_gameOverDialogShown = false;
	m_lastScore = -1;
	m_lastLives = -1;

	CreateShip();

	for (int i = 0; i < 5; ++i)
	{
		re::Vector2f pos = { (rand() % 1600 - 800) * 1.f, (rand() % 900 - 450) * 1.f };
		re::Vector2f vel = { (rand() % 200 - 100) * 1.f, (rand() % 200 - 100) * 1.f };
		SpawnAsteroid(3, pos, vel);
	}
}

void AsteroidsLayout::CreateShip()
{
	auto shipRoot = GetScene().CreateEntity();

	std::vector shipVertices = {
		re::Vertex{ .position = { 0.f, -20.f, 0.f }, .color = re::Color::Cyan },
		re::Vertex{ .position = { 15.f, 15.f, 0.f }, .color = re::Color::Cyan },
		re::Vertex{ .position = { 0.f, 10.f, 0.f }, .color = re::Color::Cyan },
		re::Vertex{ .position = { 0.f, 10.f, 0.f }, .color = re::Color::Cyan },
		re::Vertex{ .position = { -15.f, 15.f, 0.f }, .color = re::Color::Cyan },
		re::Vertex{ .position = { 0.f, -20.f, 0.f }, .color = re::Color::Cyan }
	};

	shipRoot.Add<re::TransformComponent>(re::Vector3f{ 0.f, 0.f, Z_Ship })
		.Add<re::MeshComponent>(shipVertices, re::PrimitiveType::Triangles)
		.Add<VelocityComponent>()
		.Add<ScreenWrapComponent>(20.f);

	auto flame = GetScene().CreateEntity();

	flame.Add<re::TransformComponent>(re::Vector3f{ 0.f, 0.f, Z_ShipFlame })
		.Add<re::HierarchyComponent>(shipRoot.GetEntity(), re::Vector3f{ 0.f, 0.f, 0.f });

	ShipComponent shipComp;
	shipComp.flameEntity = flame.GetEntity();
	shipRoot.Add<ShipComponent>(shipComp);

	m_shipEntity = shipRoot.GetEntity();
}

void AsteroidsLayout::SpawnAsteroid(const int size, const re::Vector2f pos, re::Vector2f vel)
{
	auto astEntity = GetScene().CreateEntity();
	astEntity.Add<re::TransformComponent>(re::Vector3f{ pos.x, pos.y, Z_Asteroid })
		.Add<VelocityComponent>(vel, (rand() % 100 - 50) * 1.5f)
		.Add<ScreenWrapComponent>(size * AsteroidBaseRadius);

	AsteroidComponent ast;
	ast.size = size;
	const int segments = AsteroidBaseSegments + size * 2;
	const float baseRadius = size * AsteroidBaseRadius;
	std::vector<re::Vertex> vertices;
	constexpr re::Color col = re::Color::Brown;

	for (int i = 0; i < segments; ++i)
	{
		const float angle = (360.0f / segments) * i;
		const float rad = angle * 3.14159f / 180.f;
		const float r = baseRadius * (0.8f + 0.4f * (rand() % 100 / 100.0f));
		ast.localPoints.push_back({ r * std::cos(rad), r * std::sin(rad) });
	}

	for (int i = 0; i < segments; ++i)
	{
		vertices.push_back({ .position = { 0.f, 0.f, 0.f }, .color = col });
		vertices.push_back({ .position = { ast.localPoints[i].x, ast.localPoints[i].y, 0.f }, .color = col });
		vertices.push_back({ .position = { ast.localPoints[(i + 1) % segments].x, ast.localPoints[(i + 1) % segments].y, 0.f }, .color = col });
	}

	astEntity.Add<re::MeshComponent>(vertices, re::PrimitiveType::Triangles);
	astEntity.Add<AsteroidComponent>(ast);
}

void AsteroidsLayout::SpawnParticle(const re::Vector2f pos, re::Vector2f vel)
{
	auto e = GetScene().CreateEntity();
	e.Add<re::TransformComponent>(re::Vector3f{ pos.x, pos.y, Z_Particle })
		.Add<VelocityComponent>(vel, (rand() % 400 - 200) * 1.f)
		.Add<ParticleComponent>(1.0f);

	std::vector v = {
		re::Vertex{ .position = { -5.f, 0.f, 0.f }, .color = re::Color::White },
		re::Vertex{ .position = { 5.f, 0.f, 0.f }, .color = re::Color::White },
		re::Vertex{ .position = { 0.f, 10.f, 0.f }, .color = re::Color::White }
	};
	e.Add<re::MeshComponent>(v, re::PrimitiveType::Triangles);
}

void AsteroidsLayout::CreateUI()
{
	auto font = m_assetManager.Get<re::Font>(FONT_NAME);

	m_scoreText = GetScene()
					  .CreateEntity()
					  .Add<re::TransformComponent>(re::Vector3f{ -800.f, -500.f, 50.f })
					  .Add<re::TextComponent>("Score: 0", font, re::Color::White, 32.f)
					  .GetEntity();

	m_livesText = GetScene()
					  .CreateEntity()
					  .Add<re::TransformComponent>(re::Vector3f{ 700.f, -500.f, 50.f })
					  .Add<re::TextComponent>("Lives: 3", font, re::Color::White, 32.f)
					  .GetEntity();
}

void AsteroidsLayout::UpdateUI()
{
	if (GetScene().IsValid(m_gameStateEntity))
	{
		const auto& [score, lives, isGameOver] = GetScene().GetComponent<AsteroidGameStateComponent>(m_gameStateEntity);

		if (score != m_lastScore || lives != m_lastLives)
		{
			m_lastScore = score;
			m_lastLives = lives;

			re::String title = "Asteroids | Score: " + std::to_string(score) + " | Lives: " + std::to_string(lives);

			if (isGameOver)
			{
				title += " [ GAME OVER ]";
			}
			m_window.SetTitle(title);
		}

		if (isGameOver && !m_gameOverDialogShown)
		{
			ShowGameOverDialog();
			m_gameOverDialogShown = true;
		}
	}
}

void AsteroidsLayout::ShowGameOverDialog()
{
	auto font = m_assetManager.Get<re::Font>(FONT_NAME);
	auto [width, height] = m_window.Size();

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector3f{ 0.f, 0.f, Z_UI_Background })
		.Add<re::RectangleComponent>(re::Color(0, 0, 0, 200), re::Vector2f{ (float)width, (float)height });

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector3f{ 0.f, -100.f, Z_UI_Text })
		.Add<re::TextComponent>("GAME OVER", font, re::Color::Red, 80.f);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector3f{ 0.f, 50.f, Z_UI_Text })
		.Add<re::TextComponent>("Play Again", font, re::Color::Green, 48.f)
		.Add<re::RectangleComponent>(re::Color::Green, re::Vector2f{ 300.f, 60.f })
		.Add<re::BoxColliderComponent>(re::Vector2f{ 300.f, 60.f }, re::Vector2f{ 0.f, 50.f })
		.Add<AsteroidButtonComponent>(AsteroidButtonComponent::Action::PlayAgain);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector3f{ 0.f, 150.f, Z_UI_Text })
		.Add<re::TextComponent>("Quit", font, re::Color::White, 48.f)
		.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 150.f, 60.f })
		.Add<re::BoxColliderComponent>(re::Vector2f{ 150.f, 60.f }, re::Vector2f{ 0.f, 150.f })
		.Add<AsteroidButtonComponent>(AsteroidButtonComponent::Action::Quit);
}

void AsteroidsLayout::OnUpdate(re::core::TimeDelta dt)
{
	UpdateUI();

	if (GetScene().IsValid(m_gameStateEntity))
	{
		const auto& [score, lives, isGameOver] = GetScene().GetComponent<AsteroidGameStateComponent>(m_gameStateEntity);
		if (!isGameOver)
		{
			int largeAsteroidsCount = 0;

			for (auto&& [entity, ast] : *GetScene().CreateView<AsteroidComponent>())
			{
				if (ast.size == 3)
				{
					largeAsteroidsCount++;
				}
			}

			const auto [halfWidth, halfHeight] = m_window.Size();
			const auto halfWidthF = static_cast<float>(halfWidth);
			const auto halfHeightF = static_cast<float>(halfHeight);
			while (largeAsteroidsCount < AsteroidMaxCount)
			{
				float x = 0.f;
				float y = 0.f;

				if (rand() % 2 == 0)
				{
					x = (rand() % 2 == 0) ? -halfWidthF : halfWidthF;
					y = (rand() % halfHeight - halfHeightF) * 1.f;
				}
				else
				{
					x = (rand() % halfWidth - halfWidthF) * 1.f;
					y = (rand() % 2 == 0) ? -halfHeightF : halfHeightF;
				}

				const re::Vector2f pos = { x, y };
				const re::Vector2f vel = { (rand() % 200 - 100) * 1.f, (rand() % 200 - 100) * 1.f };

				SpawnAsteroid(AsteroidMaxSize, pos, vel);
				largeAsteroidsCount++;
			}
		}
	}
}

void AsteroidsLayout::OnEvent(re::Event const& event)
{
	if (const auto* mousePressed = event.GetIf<re::Event::MouseButtonPressed>())
	{
		if (mousePressed->button == re::Mouse::Button::Left && m_gameOverDialogShown)
		{
			const auto mousePos = m_window.ToWorldPos(mousePressed->position);

			for (auto&& [entity, button, collider] : *GetScene().CreateView<AsteroidButtonComponent, re::BoxColliderComponent>())
			{
				if (collider.Contains(mousePos))
				{
					if (button.action == AsteroidButtonComponent::Action::PlayAgain)
					{
						StartGame();
					}
					else if (button.action == AsteroidButtonComponent::Action::Quit)
					{
						GetApplication().Shutdown();
					}
					break;
				}
			}
		}
	}
}