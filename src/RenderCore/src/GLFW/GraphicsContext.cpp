#include <RenderCore/GraphicsContext.hpp>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace re::render
{

GraphicsContext::GraphicsContext(void* windowHandle)
	: m_windowHandle(windowHandle)
{
}

void GraphicsContext::OnBeforeWindowCreate()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
}

void GraphicsContext::OnAfterWindowCreate()
{
	glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_windowHandle));
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}
}

void GraphicsContext::SetClearColor(Color color)
{
	glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
}

void GraphicsContext::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GraphicsContext::SwapBuffers()
{
	glfwSwapBuffers(static_cast<GLFWwindow*>(m_windowHandle));
}

} // namespace re::render