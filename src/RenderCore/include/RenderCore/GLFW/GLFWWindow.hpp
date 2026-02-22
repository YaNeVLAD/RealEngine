#pragma once

#include <RenderCore/Event.hpp>
#include <RenderCore/IWindow.hpp>

struct GLFWwindow; // Forward declaration

namespace re::render
{

class GLFWWindow : public IWindow
{
public:
	GLFWWindow(const std::string& title, uint32_t width, uint32_t height);
	~GLFWWindow();

	void OnUpdate() override;
	std::optional<Event> PollEvent() override;

	Vector2u Size() override
	{
		return { m_data.width, m_data.height };
	}

	void SetVSyncEnabled(bool enabled) override;

	void* GetNativeHandle() override
	{
		return m_window;
	}

	bool IsOpen() const override;

	bool SetActive(bool active) override;

	void SetTitle(const String& title) override;

	void SetIcon(const Image& image) override;

	Vector2f ToWorldPos(const Vector2i& pixelPos) override;

	void Clear() override;

	void Display() override;

private:
	void Init(const std::string& title);
	void Shutdown();

private:
	GLFWwindow* m_window{};

	struct WindowData
	{
		std::string title;
		uint32_t width, height;
		bool vsync;

		std::vector<Event> eventQueue;
	};

	WindowData m_data;
};

} // namespace re::render