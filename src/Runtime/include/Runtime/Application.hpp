#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>
#include <ECS/Scene/Scene.hpp>
#include <RenderCore/IWindow.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

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
	template <typename TScene>
	ecs::Scene& CreateScene();

	ecs::Scene& CreateScene(std::string const& name);

	template <typename TScene>
	void ChangeScene();

	void ChangeScene(std::string const& name);

	[[nodiscard]] ecs::Scene& CurrentScene() const;

	[[nodiscard]] render::IWindow& Window() const;

private:
	void GameLoop();

	ecs::Scene& AddSceneImpl(std::string const& name);

	void ChangeSceneImpl(std::string const& name);

	void ChangeToPendingScene();

private:
	std::atomic_bool m_isRunning;
	std::jthread m_gameLoopThread;

	std::mutex m_eventMutex;
	std::queue<Event> m_eventQueue;

	std::unordered_map<meta::TypeHashType, std::shared_ptr<ecs::Scene>> m_scenes;

	ecs::Scene* m_currentScene = nullptr;
	meta::TypeHashType m_pendingSceneHash = meta::InvalidTypeHash;

	std::atomic_bool m_wasResized{ false };
	std::atomic_uint32_t m_newWidth{ 0 };
	std::atomic_uint32_t m_newHeight{ 0 };

	std::unique_ptr<render::IWindow> m_window;
};

} // namespace re

#include <Runtime/Application.inl>