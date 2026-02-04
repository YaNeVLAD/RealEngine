#include <Runtime/Application.hpp>

#include "Runtime/Components.hpp"
#include "Runtime/RenderSystem2D.hpp"
#include <Render2D/Renderer2D.hpp>
#include <Render2D/SFMLRenderAPI.hpp>
#include <RenderCore/SFMLWindow.hpp>

#include <SFML/Graphics.hpp>

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

	m_scene.AddSystem<RenderSystem2D>()
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

		m_window->PollEvents();

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