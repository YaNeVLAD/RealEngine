#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <Render2D/RenderAPI/SFMLRenderAPI.hpp>
#include <Render2D/Renderer2D.hpp>
#include <RenderCore/Window/SFMLWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem2D.hpp>

namespace
{

struct DefaultTag final : re::Layout
{
	using Layout::Layout;
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

	AddLayout<DefaultTag>();
	ChangeToPendingLayout();
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
		if (m_pendingLayoutHash != meta::InvalidTypeHash)
		{
			ChangeToPendingLayout();
		}

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
				if (m_currentLayout)
				{
					m_currentLayout->OnEvent(resizeEvent);
				}
			}

			std::lock_guard lock(m_eventMutex);
			while (!m_eventQueue.empty())
			{
				auto event = m_eventQueue.front();
				m_eventQueue.pop();

				OnEvent(event);
				if (m_currentLayout)
				{
					m_currentLayout->OnEvent(event);
				}
			}
		}

		m_window->Clear();

		OnUpdate(dt);
		if (m_currentLayout)
		{
			auto& scene = m_currentLayout->GetScene();
			scene.Frame(dt);

			m_currentLayout->OnUpdate(dt);

			m_window->Display();

			scene.ConfirmChanges();
		}
	}

	m_window->SetActive(false);
}

void Application::SetupScene(Layout& layout) const
{
	auto& scene = layout.GetScene();
	scene
		.AddSystem<detail::RenderSystem2D>(*m_window)
		.WithRead<TransformComponent, RectangleComponent, CircleComponent>()
		.RunOnMainThread();

	scene
		.CreateEntity()
		.Add<CameraComponent>()
		.Add<TransformComponent>();

	scene.BuildSystemGraph();
}

void Application::SwitchLayoutImpl(std::string const& name)
{
	const auto hash = meta::HashStr(name);
	if (m_pendingLayoutHash == hash || m_currentLayoutHash == hash)
	{
		return;
	}

	if (!m_layouts.contains(hash))
	{
		return;
	}

	m_pendingLayoutHash = hash;
}

void Application::ChangeToPendingLayout()
{
	if (m_pendingLayoutHash == meta::InvalidTypeHash)
	{
		return;
	}

	if (const auto it = m_layouts.find(m_pendingLayoutHash); it != m_layouts.end())
	{
		if (m_currentLayout)
		{
			m_currentLayout->OnDetach();
		}

		if (m_currentLayout = it->second.get(); m_currentLayout)
		{
			m_currentLayout->OnAttach();
		}

		m_currentLayoutHash = m_pendingLayoutHash;
	}

	m_pendingLayoutHash = meta::InvalidTypeHash;
}

void Application::Shutdown()
{
	m_isRunning = false;
}

Layout& Application::CurrentLayout() const
{
	return *m_currentLayout;
}

render::IWindow& Application::Window() const
{
	return *m_window;
}

} // namespace re