#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <Render2D/Renderer2D.hpp>
#include <RenderCore/Internal/Input.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem2D.hpp>

#if defined(RE_USE_SFML_RENDER)
#include <RenderCore/SFML/SFMLRenderAPI.hpp>
#include <RenderCore/SFML/SFMLWindow.hpp>
#endif

#if defined(RE_USE_GLFW_RENDER)
#include <RenderCore/GLFW/GLFWWindow.hpp>
#include <RenderCore/GLFW/OpenGLRenderAPI.hpp>
#endif

#include <chrono>

namespace
{

struct DefaultTag final : re::Layout
{
	using Layout::Layout;
};

std::unique_ptr<re::render::IWindow> CreateWindow(
	std::string const& title,
	std::uint32_t width,
	std::uint32_t height)
{
#if defined(RE_USE_GLFW_RENDER)
	auto window = std::make_unique<re::render::GLFWWindow>(title, width, height);
	if (window->GetNativeHandle())
	{
		re::render::Renderer2D::Init(std::make_unique<re::render::OpenGLRenderAPI>());
		re::render::Renderer2D::SetViewport(window->Size());
		window->SetWorldPosCallback([](re::Vector2i const& pos) {
			return re::render::Renderer2D::ScreenToWorld(pos);
		});
	}
	else
	{
		throw std::runtime_error("Created window and render api are not compatible");
	}
#elif defined(RE_USE_SFML_RENDER)
	auto window = std::make_unique<re::render::SFMLWindow>(title, width, height);
	if (auto* sfWindow = window->GetSFMLWindow())
	{
		re::render::Renderer2D::Init(std::make_unique<re::render::SFMLRenderAPI>(*sfWindow));
		re::render::Renderer2D::SetViewport(window->Size());
		window->SetWorldPosCallback([](re::Vector2i const& pos) {
			return re::render::Renderer2D::ScreenToWorld(pos);
		});
	}
	else
	{
		throw std::runtime_error("Created window and render api are not compatible");
	}
#endif

	return window;
}

} // namespace

namespace re
{

Application::Application(std::string const& name)
	: m_isRunning(false)
{
	m_window = CreateWindow(name, 1920u, 1080u);
	m_window->SetVSyncEnabled(false);
	detail::Input::Init(m_window->GetNativeHandle());

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
		if (m_pendingLayoutHash != INVALID_HASH)
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
	scene.RegisterComponent<ZIndexComponent>();

	scene
		.AddSystem<detail::RenderSystem2D>(*m_window)
		.WithRead<
			TransformComponent,
			RectangleComponent,
			CircleComponent,
			DynamicTextureComponent,
			ZIndexComponent>()
		.WithWrite<DynamicTextureComponent>()
		.RunOnMainThread();

	scene
		.CreateEntity()
		.Add<CameraComponent>()
		.Add<TransformComponent>();

	scene.BuildSystemGraph();
}

void Application::SwitchLayoutImpl(const char* name)
{
	const auto hash = HashedString::Value(name, std::strlen(name));
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
	if (m_pendingLayoutHash == INVALID_HASH)
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

	m_pendingLayoutHash = INVALID_HASH;
}

void Application::Shutdown()
{
	m_isRunning = false;
}

ecs::Scene& Application::CurrentScene() const
{
	return m_currentLayout->GetScene();
}

render::IWindow& Application::Window() const
{
	return *m_window;
}

} // namespace re