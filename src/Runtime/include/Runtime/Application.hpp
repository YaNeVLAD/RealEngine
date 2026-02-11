#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>
#include <ECS/Scene/Scene.hpp>
#include <RenderCore/IWindow.hpp>
#include <Runtime/Layout.hpp>

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
	template <std::derived_from<Layout> TLayout, typename... TArgs>
	void AddLayout(TArgs&&... args);

	template <std::derived_from<Layout> TLayout>
	void SwitchLayout();

	[[nodiscard]] Layout& CurrentLayout() const;

	[[nodiscard]] render::IWindow& Window() const;

private:
	void GameLoop();

	void SetupScene(Layout& layout) const;

	void SwitchLayoutImpl(std::string const& name);

	void ChangeToPendingLayout();

private:
	std::atomic_bool m_isRunning;
	std::jthread m_gameLoopThread;

	std::mutex m_eventMutex;
	std::queue<Event> m_eventQueue;

	std::unordered_map<meta::TypeHashType, std::shared_ptr<Layout>> m_layouts;

	Layout* m_currentLayout = nullptr;
	meta::TypeHashType m_currentLayoutHash = meta::InvalidTypeHash;
	meta::TypeHashType m_pendingLayoutHash = meta::InvalidTypeHash;

	std::atomic_bool m_wasResized{ false };
	std::atomic_uint32_t m_newWidth{ 0 };
	std::atomic_uint32_t m_newHeight{ 0 };

	std::unique_ptr<render::IWindow> m_window;
};

} // namespace re

#include <Runtime/Application.inl>