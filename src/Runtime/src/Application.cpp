#include <Runtime/Application.hpp>

#include <Core/Utils.hpp>
#include <Render2D/RenderAPI/SFMLRenderAPI.hpp>
#include <Render2D/Renderer2D.hpp>
#include <RenderCore/Window/SFMLWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/RenderSystem2D.hpp>

namespace re::runtime
{

Application::Application(std::string const& name)
	: m_isRunning(false)
{
	m_window = std::make_unique<render::SFMLWindow>(name, 1920u, 1080u);

	if (auto* sfWindow = dynamic_cast<render::SFMLWindow&>(*m_window).GetSFMLWindow())
	{
		render::Renderer2D::Init(std::make_unique<render::SFMLRenderAPI>(*sfWindow));
	}
	else
	{
		throw std::runtime_error("Created window and render api are not compatible");
	}

	CurrentScene().RegisterComponents<TransformComponent, SpriteComponent, CameraComponent>();

	m_scene.AddSystem<RenderSystem2D>(*m_window)
		.WithRead<TransformComponent, SpriteComponent>()
		.RunOnMainThread();

	m_scene.CreateEntity()
		.AddComponent<CameraComponent>()
		.AddComponent<TransformComponent>();

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

	auto lastTime = std::chrono::high_resolution_clock::now();
	while (m_isRunning && m_window->IsOpen())
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		const float dt = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		while (const auto event = m_window->PollEvent())
		{
			event->Visit(utils::overloaded{
				[this](Event::Closed const&) {
					Shutdown();
				},
				[this](Event::Resized const& resized) {
					render::Renderer2D::SetViewport(resized.newSize);
				},
				[](const auto&) {} });

			OnEvent(*event);
		}

		m_window->Clear();

		m_scene.Frame(dt);

		OnUpdate(dt);

		m_window->Display();

		m_scene.ConfirmChanges();
	}

	OnStop();

	m_window.reset();
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

} // namespace re::runtime