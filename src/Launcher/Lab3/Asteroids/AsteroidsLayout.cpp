#include "AsteroidsLayout.hpp"

#include <Runtime/Application.hpp>

#include <cstdlib>
#include <ctime>

#include "AsteroidCollisionSystem.hpp"
#include "LifetimeSystem.hpp"
#include "MovementSystem.hpp"
#include "ShipSystem.hpp"

#include "../../Components.hpp"
#include "../../HierarchySystem.hpp"

namespace
{
constexpr auto FONT_NAME = "Roboto.ttf";
}

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
		.AddSystem<MovementSystem>()
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

	GetScene()
		.AddSystem<HierarchySystem>()
		.WithRead<ChildComponent>()
		.WithWrite<re::TransformComponent>();

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

	std::vector<re::Vertex> shipVertices = {
		{ { 0.f, -20.f }, re::Color::Cyan },
		{ { 15.f, 15.f }, re::Color::Cyan },
		{ { 0.f, 10.f }, re::Color::Cyan },
		{ { 0.f, 10.f }, re::Color::Cyan },
		{ { -15.f, 15.f }, re::Color::Cyan },
		{ { 0.f, -20.f }, re::Color::Cyan }
	};

	shipRoot.Add<re::TransformComponent>(re::Vector2f{ 0.f, 0.f })
		.Add<re::MeshComponent>(shipVertices, re::PrimitiveType::Triangles)
		.Add<VelocityComponent>()
		.Add<ScreenWrapComponent>(20.f)
		.Add<re::ZIndexComponent>(10);

	auto flame = GetScene().CreateEntity();

	flame.Add<re::TransformComponent>(re::Vector2f{ 0.f, 0.f })
		.Add<ChildComponent>(shipRoot.GetEntity(), re::Vector2f{ 0.f, 0.f }, 0.f)
		.Add<re::ZIndexComponent>(9);

	ShipComponent shipComp;
	shipComp.flameEntity = flame.GetEntity();
	shipRoot.Add<ShipComponent>(shipComp);

	m_shipEntity = shipRoot.GetEntity();
}

void AsteroidsLayout::SpawnAsteroid(int size, re::Vector2f pos, re::Vector2f vel)
{
	auto astEntity = GetScene().CreateEntity();
	astEntity.Add<re::TransformComponent>(pos)
		.Add<VelocityComponent>(vel, (rand() % 100 - 50) * 1.5f)
		.Add<ScreenWrapComponent>(size * 25.f)
		.Add<re::ZIndexComponent>(5);

	AsteroidComponent ast;
	ast.size = size;
	int segments = 8 + size * 2;
	float baseRadius = size * 25.0f;
	std::vector<re::Vertex> vertices;
	re::Color col = re::Color::Brown;

	for (int i = 0; i < segments; ++i)
	{
		float angle = (360.0f / segments) * i;
		float rad = angle * 3.14159f / 180.f;
		float r = baseRadius * (0.8f + 0.4f * (rand() % 100 / 100.0f));
		ast.localPoints.push_back({ r * std::cos(rad), r * std::sin(rad) });
	}

	for (int i = 0; i < segments; ++i)
	{
		vertices.push_back({ { 0.f, 0.f }, col });
		vertices.push_back({ ast.localPoints[i], col });
		vertices.push_back({ ast.localPoints[(i + 1) % segments], col });
	}

	astEntity.Add<re::MeshComponent>(vertices, re::PrimitiveType::Triangles);
	astEntity.Add<AsteroidComponent>(ast);
}

void AsteroidsLayout::SpawnParticle(re::Vector2f pos, re::Vector2f vel)
{
	auto e = GetScene().CreateEntity();
	e.Add<re::TransformComponent>(pos)
		.Add<VelocityComponent>(vel, (rand() % 400 - 200) * 1.f)
		.Add<ParticleComponent>(1.0f)
		.Add<re::ZIndexComponent>(15);

	std::vector<re::Vertex> v = {
		{ { -5.f, 0.f }, re::Color::White },
		{ { 5.f, 0.f }, re::Color::White },
		{ { 0.f, 10.f }, re::Color::White }
	};
	e.Add<re::MeshComponent>(v, re::PrimitiveType::Triangles);
}

void AsteroidsLayout::CreateUI()
{
	auto font = m_assetManager.Get<re::Font>(FONT_NAME);

	m_scoreText = GetScene()
					  .CreateEntity()
					  .Add<re::TransformComponent>(re::Vector2f{ -800.f, -500.f })
					  .Add<re::TextComponent>("Score: 0", font, re::Color::White, 32.f)
					  .Add<re::ZIndexComponent>(50)
					  .GetEntity();

	m_livesText = GetScene()
					  .CreateEntity()
					  .Add<re::TransformComponent>(re::Vector2f{ 700.f, -500.f })
					  .Add<re::TextComponent>("Lives: 3", font, re::Color::White, 32.f)
					  .Add<re::ZIndexComponent>(50)
					  .GetEntity();
}

void AsteroidsLayout::UpdateUI()
{
	if (GetScene().IsValid(m_gameStateEntity))
	{
		const auto& state = GetScene().GetComponent<AsteroidGameStateComponent>(m_gameStateEntity);
		GetScene().GetComponent<re::TextComponent>(m_scoreText).text = "Score: " + std::to_string(state.score);
		GetScene().GetComponent<re::TextComponent>(m_livesText).text = "Lives: " + std::to_string(state.lives);

		if (state.isGameOver && !m_gameOverDialogShown)
		{
			ShowGameOverDialog();
			m_gameOverDialogShown = true;
		}
	}
}

void AsteroidsLayout::ShowGameOverDialog()
{
	auto font = m_assetManager.Get<re::Font>(FONT_NAME);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0.f, 0.f })
		.Add<re::RectangleComponent>(re::Color(0, 0, 0, 200), re::Vector2f{ 1920.f, 1080.f })
		.Add<re::ZIndexComponent>(90);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0.f, -100.f })
		.Add<re::TextComponent>("GAME OVER", font, re::Color::Red, 80.f)
		.Add<re::ZIndexComponent>(100);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0.f, 50.f })
		.Add<re::TextComponent>("Play Again", font, re::Color::Green, 48.f)
		.Add<re::BoxColliderComponent>(re::Vector2f{ 300.f, 60.f }, re::Vector2f{ 0.f, 50.f })
		.Add<AsteroidButtonComponent>(AsteroidButtonComponent::Action::PlayAgain)
		.Add<re::ZIndexComponent>(100);

	GetScene()
		.CreateEntity()
		.Add<re::TransformComponent>(re::Vector2f{ 0.f, 150.f })
		.Add<re::TextComponent>("Quit", font, re::Color::White, 48.f)
		.Add<re::BoxColliderComponent>(re::Vector2f{ 150.f, 60.f }, re::Vector2f{ 0.f, 150.f })
		.Add<AsteroidButtonComponent>(AsteroidButtonComponent::Action::Quit)
		.Add<re::ZIndexComponent>(100);
}

void AsteroidsLayout::OnUpdate(re::core::TimeDelta dt)
{
	UpdateUI();
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