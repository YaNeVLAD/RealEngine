#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <GUI/Context.hpp>
#include <Physics/Core.hpp>
#include <Render2D/Renderer2D.hpp>
#include <Render3D/Renderer3D.hpp>
#include <RenderCore/Filament/FilamentRenderBackend.hpp>
#include <RenderCore/GLFW/GLFWWindow.hpp>
#include <RenderCore/Internal/Input.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem3D.hpp>
#include <Runtime/System/HierarchySystem.hpp>
#include <Runtime/System/PhysicsSystem.hpp>

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
	auto window = std::make_unique<re::render::GLFWWindow>(title, width, height);
	if (!window->GetNativeHandle())
	{
		throw std::runtime_error("Failed to create GLFW window for Filament");
	}
	window->SetWorldPosCallback([](re::Vector2i const& pos) {
		return re::render::Renderer2D::ScreenToWorld(pos);
	});

	return window;
}

} // namespace

namespace re
{

Application::Application(std::string const& name)
	: Application(name, CreateWindow(name, 1920u, 1080u))
{
}

Application::Application(std::string const& name, std::unique_ptr<render::IWindow> window)
	: m_isRunning(false)
	, m_window(std::move(window))
{
	(void)name;

	m_renderBackend = Application::CreateBackend();

	physics::Init();

	AddLayout<DefaultTag>();
	ChangeToPendingLayout();
}

Application::~Application()
{
	Shutdown();
}

void Application::Run()
{
	InitInput();

	m_isRunning = true;

	OnStart();

	m_window->SetActive(false);

	m_gameLoopThread = std::jthread(&Application::GameLoop, this);

	while (m_isRunning)
	{
		if (m_pendingCursorLockUpdate.exchange(false))
		{
			m_window->SetCursorLocked(m_nextCursorLockState.load());
		}

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

	gui::Context::Init(m_window->GetNativeHandle());

	if (m_renderBackend)
	{
		m_renderBackend->Init(m_window->GetOSWindowHandle(), m_window->Size().x, m_window->Size().y);
	}

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

				if (m_renderBackend)
				{
					m_renderBackend->Resize(newSize.x, newSize.y);
				}

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

				if (!gui::Context::ProcessEvent(event))
				{
					OnEvent(event);
					if (m_currentLayout)
					{
						m_currentLayout->OnEvent(event);
					}
				}
			}
		}

		m_window->Clear();

		Frame(dt);

		m_window->Display();
	}

	m_renderBackend->Shutdown();

	gui::Context::Shutdown();

	m_window->SetActive(false);
}

void Application::SetupScene(Layout& layout) const
{
	auto& scene = layout.GetScene();
	scene
		.AddSystem<PhysicsSystem>(scene)
		.WithRead<TransformComponent, physics::RigidBody>()
		.WithWrite<TransformComponent>()
		.RunOnMainThread();

	scene.AddSystem<HierarchySystem>()
		.WithRead<HierarchyComponent, TransformComponent>()
		.WithWrite<TransformComponent>()
		.RunOnMainThread();

	scene
		.AddSystem<render::RenderSystem3D>(*m_renderBackend)
		.WithRead<
			TransformComponent,
			StaticMeshComponent3D,
			LightComponent,
			CameraComponent,
			SkyboxComponent,
			detail::DirtyTag<TransformComponent>>()
		.RunOnMainThread();

	scene
		.CreateEntity()
		.Add<detail::DirtyTag<TransformComponent>>()
		.Add<TransformComponent>({ .rotation = Vector3f{ 0.f, -90.f, 0.f } })
		.Add<CameraComponent>()
		.Add<NameComponent>("Main Camera");

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
	physics::Shutdown();
}

void Application::SetUIOverlayActive(const bool active)
{
	m_isUiOverlayActive = active;

	m_nextCursorLockState = !active;
	m_pendingCursorLockUpdate = true;

	gui::Context::SetInteractive(active);
}

bool Application::IsUIOverlayActive() const
{
	return m_isUiOverlayActive;
}

ecs::Scene& Application::CurrentScene() const
{
	return m_currentLayout->GetScene();
}

render::IWindow& Application::Window() const
{
	return *m_window;
}

void Application::SetVSyncEnabled(const bool enabled) const
{
	m_window->SetVSyncEnabled(enabled);
	if (m_renderBackend)
	{
		m_renderBackend->SetVSyncEnabled(enabled, m_window->GetRefreshRateHz());
	}
}

void Application::Frame(const float dt)
{
	if (m_pendingLayoutHash != INVALID_HASH)
	{
		ChangeToPendingLayout();
	}

	if (m_renderBackend)
	{
		m_renderBackend->SetClearColor(m_window->GetBackgroundColor());
	}

	OnUpdate(dt);
	if (!m_currentLayout)
	{
		return;
	}

	auto& scene = m_currentLayout->GetScene();

	if (m_renderBackend)
	{
		m_renderBackend->BeginFrame();
	}

	scene.Frame(dt);

	m_currentLayout->OnUpdate(dt);

	DrawOverlay(dt);

	if (m_renderBackend)
	{
		m_renderBackend->RenderFrame();
	}

	scene.ConfirmChanges();

	if (m_renderBackend)
	{
		m_renderBackend->EndFrame();
	}
}

void Application::DrawOverlay(const float dt)
{
	gui::Context::BeginFrame();

	if (m_renderBackend && m_currentLayout)
	{
		m_renderBackend->RenderUI(dt, [&]() {
			m_currentLayout->OnUIDraw();
		});
	}
}

std::unique_ptr<render::IRenderBackend> Application::CreateBackend() const
{
	return std::make_unique<render::FilamentRenderBackend>();
}

void Application::InitInput() const
{
	detail::Input::Init(m_window->GetNativeHandle());
}

render::IRenderBackend& Application::Backend() const
{
	return *m_renderBackend;
}

} // namespace re
