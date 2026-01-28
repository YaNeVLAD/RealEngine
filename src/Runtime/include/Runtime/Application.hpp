#pragma once

#include <Runtime/Export.hpp>

#include <Core/Types.hpp>
#include <Runtime/Scene.hpp>

#include <memory>

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
	Application();
	~Application();

	void Initialize(const std::string& title, uint32_t width, uint32_t height);
	void Run();
	void Shutdown();

	Scene& CurrentScene();

private:
	void Update(core::TimeDelta dt);
	void Render();

private:
	std::unique_ptr<IWindow> m_window;
	Scene m_scene;
	bool m_isRunning;
};

} // namespace re::runtime