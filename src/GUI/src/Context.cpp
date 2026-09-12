#include <GUI/Context.hpp>

#include <imgui.h>

#if defined(RE_USE_FILAMENT_RENDER)
#include <backends/imgui_impl_glfw.h>
struct GLFWwindow;
#endif

#if defined(RE_USE_SFML_RENDER)
// TODO: Add support for SFML
#endif

namespace re::gui::Context
{

void Init(void* nativeWindowHandle)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

#if defined(RE_USE_FILAMENT_RENDER)
	ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(nativeWindowHandle), true);

	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
#endif
}

void Shutdown()
{
#if defined(RE_USE_FILAMENT_RENDER)
	ImGui_ImplGlfw_Shutdown();
#endif

	ImGui::DestroyContext();
}

void BeginFrame()
{
#if defined(RE_USE_FILAMENT_RENDER)
	ImGui_ImplGlfw_NewFrame();
#endif

	ImGui::NewFrame();
}

void EndFrame()
{
	ImGui::Render();
}

bool ProcessEvent(const Event& event)
{
	const ImGuiIO& io = ImGui::GetIO();
	bool handled = false;

	if (event.GetIf<Event::MouseButtonPressed>() || event.GetIf<Event::MouseButtonReleased>() || event.GetIf<Event::MouseMoved>())
	{
		handled = io.WantCaptureMouse;
	}
	else if (event.GetIf<Event::KeyPressed>() || event.GetIf<Event::KeyReleased>() || event.GetIf<Event::TextEntered>())
	{
		handled = io.WantCaptureKeyboard;
	}

	return handled;
}

void SetInteractive(const bool interactive)
{
	ImGuiIO& io = ImGui::GetIO();
	if (interactive)
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
	else
	{
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}
}

void* GetDrawData()
{
	return ImGui::GetDrawData();
}

} // namespace re::gui::Context