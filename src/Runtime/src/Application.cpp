#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <Render2D/RenderAPI/SFMLRenderAPI.hpp>
#include <Render2D/Renderer2D.hpp>
#include <RenderCore/Window/SFMLWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem2D.hpp>

namespace re
{

Application::Application(std::string const& name)
	: m_isRunning(false)
{
	m_window = std::make_unique<render::SFMLWindow>(name, 1024u, 680u);

	if (auto* sfWindow = dynamic_cast<render::SFMLWindow&>(*m_window).GetSFMLWindow())
	{
		render::Renderer2D::Init(std::make_unique<render::SFMLRenderAPI>(*sfWindow));
	}
	else
	{
		throw std::runtime_error("Created window and render api are not compatible");
	}

	m_scene.AddSystem<detail::RenderSystem2D>(*m_window)
		.WithRead<TransformComponent, RectangleComponent, CircleComponent>()
		.RunOnMainThread();

	m_scene.CreateEntity()
		.Add<CameraComponent>()
		.Add<TransformComponent>();

	m_scene.BuildSystemGraph();
}

Application::~Application()
{
	Shutdown();
}

void Application::Run()
{
	m_isRunning = true;

	OnStart();

	m_window->SetActive(false);

	m_gameLoopThread = std::jthread(&Application::GameLoop, this);

	while (m_isRunning)
	{
		while (const auto event = m_window->PollEvent())
		{
			if (const auto* resized = event->GetIf<Event::Resized>())
			{
				m_newWidth = resized->newSize.x;
				m_newHeight = resized->newSize.y;
				m_wasResized = true;

				continue;
			}

			event->Visit(utils::overloaded{
				[this](Event::Closed const&) {
					Shutdown();
				},
				[](const auto&) {} });

			{
				std::lock_guard lock(m_eventMutex);
				m_eventQueue.push(*event);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	OnStop();

	if (m_gameLoopThread.joinable())
	{
		m_gameLoopThread.join();
	}

	m_window.reset();
}

void Application::GameLoop()
{
	m_window->SetActive(true);

	auto lastTime = std::chrono::high_resolution_clock::now();
	while (m_isRunning)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		const float dt = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		{
			if (m_wasResized.exchange(false))
			{
				Vector2u newSize = { m_newWidth, m_newHeight };
				render::Renderer2D::SetViewport(newSize);

				Event resizeEvent(Event::Resized{ newSize });
				OnEvent(resizeEvent);
			}

			std::lock_guard lock(m_eventMutex);
			while (!m_eventQueue.empty())
			{
				auto event = m_eventQueue.front();
				m_eventQueue.pop();

				OnEvent(event);
			}
		}

		m_window->Clear();

		m_scene.Frame(dt);
		OnUpdate(dt);

		m_window->Display();

		m_scene.ConfirmChanges();
	}

	m_window->SetActive(false);
}

void Application::Shutdown()
{
	m_isRunning = false;
}

ecs::Scene& Application::CurrentScene()
{
	return m_scene;
}

render::IWindow& Application::Window() const
{
	return *m_window;
}

} // namespace re