#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <Render2D/RenderAPI/SFMLRenderAPI.hpp>
#include <Render2D/Renderer2D.hpp>
#include <RenderCore/Window/SFMLWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem2D.hpp>

namespace
{

struct DefaultTag
{
};

} // namespace

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

	CreateScene<DefaultTag>();
	ChangeToPendingScene();
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
		ChangeToPendingScene();

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

		if (m_currentScene)
		{
			m_currentScene->Frame(dt);
			OnUpdate(dt);

			m_window->Display();

			m_currentScene->ConfirmChanges();
		}
	}

	m_window->SetActive(false);
}

ecs::Scene& Application::AddSceneImpl(std::string const& name)
{
	const auto scenesCount = m_scenes.size();
	const auto hash = meta::HashStr(name);

	if (m_scenes.contains(hash))
	{
		return *m_scenes[hash];
	}

	const auto newScene = std::make_shared<ecs::Scene>();
	newScene
		->AddSystem<detail::RenderSystem2D>(*m_window)
		.WithRead<TransformComponent, RectangleComponent, CircleComponent>()
		.RunOnMainThread();

	newScene
		->CreateEntity()
		.Add<CameraComponent>()
		.Add<TransformComponent>();

	newScene->BuildSystemGraph();

	auto& scene = *(m_scenes[hash] = newScene);

	if (scenesCount == 0)
	{
		ChangeScene(name);
	}

	return scene;
}

void Application::ChangeSceneImpl(std::string const& name)
{
	const auto hash = meta::HashStr(name);
	if (!m_scenes.contains(hash))
	{
		return;
	}

	m_pendingSceneHash = hash;
}

void Application::ChangeToPendingScene()
{
	if (m_pendingSceneHash == meta::InvalidTypeHash)
	{
		return;
	}

	if (const auto it = m_scenes.find(m_pendingSceneHash); it != m_scenes.end())
	{
		m_currentScene = it->second.get();
	}

	m_pendingSceneHash = meta::InvalidTypeHash;
}

void Application::Shutdown()
{
	m_isRunning = false;
}

ecs::Scene& Application::CreateScene(std::string const& name)
{
	return AddSceneImpl(name);
}

void Application::ChangeScene(std::string const& name)
{
	ChangeSceneImpl(name);
}

ecs::Scene& Application::CurrentScene() const
{
	return *m_currentScene;
}

render::IWindow& Application::Window() const
{
	return *m_window;
}

} // namespace re