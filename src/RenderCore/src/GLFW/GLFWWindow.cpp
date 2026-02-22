#include <RenderCore/GLFW/GLFWWindow.hpp>

#include <glad/glad.h> // Include glad before any OpenGL library

#include <GLFW/glfw3.h>

#include <iostream>

namespace re::render
{

static bool s_glfwInitialized = false;

GLFWWindow::GLFWWindow(const std::string& title, uint32_t width, uint32_t height)
{
	m_data.title = title;
	m_data.width = width;
	m_data.height = height;
	Init(title);
}

GLFWWindow::~GLFWWindow()
{
	Shutdown();
}

void GLFWWindow::Init(const std::string& title)
{
	if (!s_glfwInitialized)
	{
		int success = glfwInit();
		// check success...
		s_glfwInitialized = true;
	}

	// Настройка OpenGL 4.5 Core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow((int)m_data.width, (int)m_data.height, title.c_str(), nullptr, nullptr);

	// Делаем контекст активным
	glfwMakeContextCurrent(m_window);

	// Инициализируем GLAD (загружаем функции OpenGL)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Связываем этот объект окна с GLFW окном
	glfwSetWindowUserPointer(m_window, &m_data);

	SetVSyncEnabled(true);

	// --- Установка Коллбеков ---

	// Resize
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		data.width = width;
		data.height = height;

		data.eventQueue.emplace_back(Event::Resized{ (uint32_t)width, (uint32_t)height });
	});

	// Close
	glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		data.eventQueue.emplace_back(Event::Closed{});
	});

	// Key
	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

		// Тут нужно конвертировать GLFW key в re::KeyCode
		// re::KeyCode engineKey = TranslateGlfwKey(key);

		if (action == GLFW_PRESS)
		{
			data.eventQueue.emplace_back(Event::KeyPressed{ Keyboard::Key::Space, false });
		}
		else if (action == GLFW_RELEASE)
		{
			// ...
		}
	});
}

void GLFWWindow::Shutdown()
{
}

void GLFWWindow::OnUpdate()
{
	glfwPollEvents();
	glfwSwapBuffers(m_window);
}

std::optional<Event> GLFWWindow::PollEvent()
{
	if (m_data.eventQueue.empty())
		return std::nullopt;

	Event e = m_data.eventQueue.front();
	m_data.eventQueue.erase(m_data.eventQueue.begin());
	return e;
}

void GLFWWindow::SetVSyncEnabled(bool enabled)
{
}

bool GLFWWindow::IsOpen() const
{
	return true;
}

bool GLFWWindow::SetActive(bool active)
{
	return true;
}

void GLFWWindow::SetTitle(const String& title)
{
}

void GLFWWindow::SetIcon(const Image& image)
{
}

Vector2f GLFWWindow::ToWorldPos(const Vector2i& pixelPos)
{
	return {};
}

void GLFWWindow::Clear()
{
}

void GLFWWindow::Display()
{
}

// ... Реализация SetVSync (glfwSwapInterval) ...

} // namespace re::render