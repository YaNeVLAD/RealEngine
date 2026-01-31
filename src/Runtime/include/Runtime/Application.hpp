#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>
#include <ECS/Scene/Scene.hpp>

#include <memory>
#include <string>

namespace re::runtime
{

class RE_RUNTIME_API IWindow
{
public:
	virtual ~IWindow() = default;
	virtual bool IsOpen() const = 0;
	virtual void PollEvents() = 0;
	virtual void Clear() = 0;
	virtual void Display() = 0;
	virtual void* GetNativeHandle() = 0;
};

class RE_RUNTIME_API Application
{
public:
	explicit Application(std::string const& name);

	virtual ~Application();

	void Run();

	void Shutdown();

	virtual void OnStart() = 0;

	virtual void OnUpdate(core::TimeDelta deltaTime) = 0;

	virtual void OnStop() = 0;

protected:
	[[nodiscard]] ecs::Scene& CurrentScene();

	[[nodiscard]] IWindow& Window() const;

private:
	bool m_isRunning;

	ecs::Scene m_scene;

	std::unique_ptr<IWindow> m_window;
};

} // namespace re::runtime