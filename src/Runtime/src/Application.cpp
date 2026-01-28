#include <Runtime/Application.hpp>

#include <SFML/Graphics.hpp>

#include <iostream>

namespace re::runtime
{
class SfmlWindow final : public IWindow
{
public:
	SfmlWindow(std::string const& title, const uint32_t width, const uint32_t height)
	{
		m_renderWindow.create(sf::VideoMode(sf::Vector2u{ width, height }), title);
	}

	bool IsOpen() const override
	{
		return m_renderWindow.isOpen();
	}

	void PollEvents() override
	{
		while (const auto event = m_renderWindow.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				m_renderWindow.close();
		}
	}

	void Clear() override
	{
		m_renderWindow.clear();
	}

	void Display() override
	{
		m_renderWindow.display();
	}

	void* GetNativeHandle() override
	{
		return m_renderWindow.getNativeHandle();
	}

	sf::RenderWindow& GetSFMLWindow()
	{
		return m_renderWindow;
	}

private:
	sf::RenderWindow m_renderWindow;
};

Application::Application()
	: m_isRunning(false)
{
}

Application::~Application()
{
	Shutdown();
}

void Application::Initialize(const std::string& title, uint32_t width, uint32_t height)
{
	m_window = std::make_unique<SfmlWindow>(title, width, height);
	m_isRunning = true;

	// Пример: создание сущности при инициализации
	// auto entity = m_scene.CreateEntity();
	// m_registry.emplace<TransformComponent>(entity, ...);
}

void Application::Run()
{
	sf::Clock clock;
	while (m_isRunning && m_window->IsOpen())
	{
		const core::TimeDelta dt = clock.restart().asSeconds();

		m_window->PollEvents();

		Update(dt);
		Render();
	}
}

void Application::Shutdown()
{
	m_isRunning = false;
	m_scene.Clear();
}

Scene& Application::CurrentScene()
{
	return m_scene;
}

void Application::Update(core::TimeDelta dt)
{
	// Здесь будет логика систем ECS
	// Например: SystemPhysics(m_registry, dt);
}

void Application::Render()
{
	m_window->Clear();

	// В будущем здесь будет проход по RenderSystem
	// auto view = m_registry.view<RenderComponent, TransformComponent>();

	m_window->Display();
}
} // namespace re::runtime