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

Application::Application(std::string const& name)
	: m_isRunning(false)
{
	m_window = std::make_unique<SfmlWindow>(name, 1920u, 1080u);
}

Application::~Application()
{
	Shutdown();
}

void Application::Run()
{
	m_isRunning = true;

	OnStart();

	auto lastTime = std::chrono::high_resolution_clock::now();
	while (m_isRunning && m_window->IsOpen())
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		const float dt = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		m_window->PollEvents();

		m_window->Clear();

		m_scene.OnUpdate(dt);

		OnUpdate(dt);

		m_window->Display();
	}

	OnStop();

	m_window.reset();
	m_scene.Clear();
}

void Application::Shutdown()
{
	m_isRunning = false;
}

Scene& Application::CurrentScene()
{
	return m_scene;
}

IWindow& Application::Window() const
{
	return *m_window;
}

} // namespace re::runtime