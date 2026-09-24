#pragma once

#include <Runtime/Export.hpp>

#include <Core/HashedString.hpp>
#include <Core/Types.hpp>
#include <ECS/Scene.hpp>
#include <RenderCore/IWindow.hpp>
#include <RenderCore/Interface/IRenderBackend.hpp>
#include <Runtime/Layout.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace re::render
{
class IRenderBackend;
}

namespace re
{

class RE_RUNTIME_API Application
{
public:
	explicit Application(std::string const& name);
	Application(std::string const& name, std::unique_ptr<render::IWindow> window);

	virtual ~Application();

	void Run();

	void Shutdown();

	void Frame(float dt);

	virtual void OnStart() = 0;

	virtual void OnUpdate(core::TimeDelta deltaTime) = 0;

	virtual void OnEvent(Event const&) {}

	virtual void OnStop() = 0;

	void SetVSyncEnabled(bool enabled) const;

	void SetUIOverlayActive(bool active);

	bool IsUIOverlayActive() const;

protected:
	template <std::derived_from<Layout> TLayout, typename... TArgs>
	TLayout& AddLayout(TArgs&&... args);

	template <std::derived_from<Layout> TLayout>
	void SwitchLayout();

	[[nodiscard]] ecs::Scene& CurrentScene() const;

	[[nodiscard]] render::IWindow& Window() const;

	[[nodiscard]] render::IRenderBackend& Backend() const;

	virtual void SetupScene(Layout& layout) const;

	virtual std::unique_ptr<render::IRenderBackend> CreateBackend() const;

	virtual void InitInput() const;

	virtual void DrawOverlay(float dt);

private:
	void GameLoop();

	void SwitchLayoutImpl(const char* name);

	void ChangeToPendingLayout();

private:
	std::atomic_bool m_isRunning;
	std::jthread m_gameLoopThread;

	std::mutex m_eventMutex;
	std::queue<Event> m_eventQueue;

	std::unordered_map<Hash_t, std::shared_ptr<Layout>> m_layouts;

	Layout* m_currentLayout = nullptr;
	Hash_t m_currentLayoutHash = INVALID_HASH;
	Hash_t m_pendingLayoutHash = INVALID_HASH;

	std::atomic_bool m_wasResized{ false };
	std::atomic_uint32_t m_newWidth{ 0 };
	std::atomic_uint32_t m_newHeight{ 0 };

	bool m_isUiOverlayActive = true;

	std::atomic_bool m_pendingCursorLockUpdate{ false };
	std::atomic_bool m_nextCursorLockState{ false };

	std::unique_ptr<render::IWindow> m_window;

	std::unique_ptr<render::IRenderBackend> m_renderBackend;
};

} // namespace re

#include <Runtime/Application.inl>
