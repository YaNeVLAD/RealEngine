#pragma once

#include <RenderCore/Event.hpp>
#include <RenderCore/IWindow.hpp>

struct GLFWwindow; // Forward declaration

namespace re::render
{

class RE_RENDER_CORE_API GLFWWindow final : public IWindow
{
public:
	GLFWWindow(const std::string& title, uint32_t width, uint32_t height);
	~GLFWWindow() override;

	std::optional<Event> PollEvent() override;

	Vector2u Size() override;

	void SetVSyncEnabled(bool enabled) override;

	void* GetNativeHandle() override;

	bool IsOpen() const override;

	bool SetActive(bool active) override;

	void SetTitle(const String& title) override;

	void SetIcon(const Image& image) override;

	Vector2f ToWorldPos(const Vector2i& pixelPos) override;

	void Clear() override;

	void Display() override;

	void SetWorldPosCallback(WorldPosCallback&& callback) override;

private:
	void Init(const std::string& title);
	void Shutdown();

private:
	GLFWwindow* m_window{};

	struct WindowData
	{
		String title;
		std::uint32_t width;
		std::uint32_t height;
		bool vsync;

		std::vector<Event> eventQueue;
	};

	WindowData m_data;
	WorldPosCallback m_worldPosCallback;
};

} // namespace re::render