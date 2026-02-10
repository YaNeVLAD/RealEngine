#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>
#include <ECS/Scene/Scene.hpp>
#include <RenderCore/IWindow.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace re
{

class RE_RUNTIME_API Application
{
public:
	explicit Application(std::string const& name);

	virtual ~Application();

	void Run();

	void Shutdown();

	virtual void OnStart() = 0;

	virtual void OnUpdate(core::TimeDelta deltaTime) = 0;

	virtual void OnEvent(Event const&) {}

	virtual void OnStop() = 0;

protected:
	[[nodiscard]] ecs::Scene& CurrentScene();

	[[nodiscard]] render::IWindow& Window() const;

private:
	void GameLoop();

private:
	std::atomic_bool m_isRunning;
	std::jthread m_gameLoopThread;

	std::mutex m_eventMutex;
	std::queue<Event> m_eventQueue;

	ecs::Scene m_scene;

	std::atomic_bool m_wasResized{ false };
	std::atomic_uint32_t m_newWidth{ 0 };
	std::atomic_uint32_t m_newHeight{ 0 };

	std::unique_ptr<render::IWindow> m_window;
};

} // namespace re