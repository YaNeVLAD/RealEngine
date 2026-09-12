#include <RenderCore/GraphicsContext.hpp>

#include <GLFW/glfw3.h>

namespace re::render
{

GraphicsContext::GraphicsContext(void* windowHandle)
	: m_windowHandle(windowHandle)
{
}

void GraphicsContext::OnBeforeWindowCreate()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void GraphicsContext::OnAfterWindowCreate()
{
}

void GraphicsContext::SetClearColor([[maybe_unused]] Color color)
{
}

void GraphicsContext::Clear()
{
}

void GraphicsContext::SwapBuffers()
{
}

} // namespace re::render