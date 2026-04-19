#include <GUI/Context.hpp>

#include <imgui.h>

#ifdef RE_USE_GLFW_RENDER
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#elif defined(RE_USE_SFML_RENDER)
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

	ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(nativeWindowHandle), true);
	ImGui_ImplOpenGL3_Init("#version 450 core");
}

void Shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void BeginFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void EndFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

} // namespace re::gui::Context