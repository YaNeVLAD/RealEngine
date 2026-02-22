#include <RenderCore/GLFW/GLFWWindow.hpp>

#include <glad/glad.h> // Include glad before any OpenGL library

#include <GLFW/glfw3.h>

#include <iostream>

namespace
{

constexpr re::Keyboard::Key GlfwKeyToKey(const int glfwKey)
{
	// clang-format off
    switch (glfwKey)
    {
        case GLFW_KEY_A:            return re::Keyboard::Key::A;
        case GLFW_KEY_B:            return re::Keyboard::Key::B;
        case GLFW_KEY_C:            return re::Keyboard::Key::C;
        case GLFW_KEY_D:            return re::Keyboard::Key::D;
        case GLFW_KEY_E:            return re::Keyboard::Key::E;
        case GLFW_KEY_F:            return re::Keyboard::Key::F;
        case GLFW_KEY_G:            return re::Keyboard::Key::G;
        case GLFW_KEY_H:            return re::Keyboard::Key::H;
        case GLFW_KEY_I:            return re::Keyboard::Key::I;
        case GLFW_KEY_J:            return re::Keyboard::Key::J;
        case GLFW_KEY_K:            return re::Keyboard::Key::K;
        case GLFW_KEY_L:            return re::Keyboard::Key::L;
        case GLFW_KEY_M:            return re::Keyboard::Key::M;
        case GLFW_KEY_N:            return re::Keyboard::Key::N;
        case GLFW_KEY_O:            return re::Keyboard::Key::O;
        case GLFW_KEY_P:            return re::Keyboard::Key::P;
        case GLFW_KEY_Q:            return re::Keyboard::Key::Q;
        case GLFW_KEY_R:            return re::Keyboard::Key::R;
        case GLFW_KEY_S:            return re::Keyboard::Key::S;
        case GLFW_KEY_T:            return re::Keyboard::Key::T;
        case GLFW_KEY_U:            return re::Keyboard::Key::U;
        case GLFW_KEY_V:            return re::Keyboard::Key::V;
        case GLFW_KEY_W:            return re::Keyboard::Key::W;
        case GLFW_KEY_X:            return re::Keyboard::Key::X;
        case GLFW_KEY_Y:            return re::Keyboard::Key::Y;
        case GLFW_KEY_Z:            return re::Keyboard::Key::Z;
        case GLFW_KEY_0:            return re::Keyboard::Key::Num0;
        case GLFW_KEY_1:            return re::Keyboard::Key::Num1;
        case GLFW_KEY_2:            return re::Keyboard::Key::Num2;
        case GLFW_KEY_3:            return re::Keyboard::Key::Num3;
        case GLFW_KEY_4:            return re::Keyboard::Key::Num4;
        case GLFW_KEY_5:            return re::Keyboard::Key::Num5;
        case GLFW_KEY_6:            return re::Keyboard::Key::Num6;
        case GLFW_KEY_7:            return re::Keyboard::Key::Num7;
        case GLFW_KEY_8:            return re::Keyboard::Key::Num8;
        case GLFW_KEY_9:            return re::Keyboard::Key::Num9;
        case GLFW_KEY_ESCAPE:       return re::Keyboard::Key::Escape;
        case GLFW_KEY_LEFT_CONTROL:  return re::Keyboard::Key::LControl;
        case GLFW_KEY_LEFT_SHIFT:    return re::Keyboard::Key::LShift;
        case GLFW_KEY_LEFT_ALT:      return re::Keyboard::Key::LAlt;
        case GLFW_KEY_LEFT_SUPER:    return re::Keyboard::Key::LSystem;
        case GLFW_KEY_RIGHT_CONTROL: return re::Keyboard::Key::RControl;
        case GLFW_KEY_RIGHT_SHIFT:   return re::Keyboard::Key::RShift;
        case GLFW_KEY_RIGHT_ALT:     return re::Keyboard::Key::RAlt;
        case GLFW_KEY_RIGHT_SUPER:   return re::Keyboard::Key::RSystem;
        case GLFW_KEY_MENU:          return re::Keyboard::Key::Menu;
        case GLFW_KEY_LEFT_BRACKET:  return re::Keyboard::Key::LBracket;
        case GLFW_KEY_RIGHT_BRACKET: return re::Keyboard::Key::RBracket;
        case GLFW_KEY_SEMICOLON:     return re::Keyboard::Key::Semicolon;
        case GLFW_KEY_COMMA:         return re::Keyboard::Key::Comma;
        case GLFW_KEY_PERIOD:        return re::Keyboard::Key::Period;
        case GLFW_KEY_APOSTROPHE:    return re::Keyboard::Key::Apostrophe;
        case GLFW_KEY_SLASH:         return re::Keyboard::Key::Slash;
        case GLFW_KEY_BACKSLASH:     return re::Keyboard::Key::Backslash;
        case GLFW_KEY_GRAVE_ACCENT:  return re::Keyboard::Key::Grave;
        case GLFW_KEY_EQUAL:         return re::Keyboard::Key::Equal;
        case GLFW_KEY_MINUS:         return re::Keyboard::Key::Hyphen;
        case GLFW_KEY_SPACE:         return re::Keyboard::Key::Space;
        case GLFW_KEY_ENTER:         return re::Keyboard::Key::Enter;
        case GLFW_KEY_BACKSPACE:     return re::Keyboard::Key::Backspace;
        case GLFW_KEY_TAB:           return re::Keyboard::Key::Tab;
        case GLFW_KEY_PAGE_UP:       return re::Keyboard::Key::PageUp;
        case GLFW_KEY_PAGE_DOWN:     return re::Keyboard::Key::PageDown;
        case GLFW_KEY_END:           return re::Keyboard::Key::End;
        case GLFW_KEY_HOME:          return re::Keyboard::Key::Home;
        case GLFW_KEY_INSERT:        return re::Keyboard::Key::Insert;
        case GLFW_KEY_DELETE:        return re::Keyboard::Key::Delete;
        case GLFW_KEY_KP_ADD:        return re::Keyboard::Key::Add;
        case GLFW_KEY_KP_SUBTRACT:   return re::Keyboard::Key::Subtract;
        case GLFW_KEY_KP_MULTIPLY:   return re::Keyboard::Key::Multiply;
        case GLFW_KEY_KP_DIVIDE:     return re::Keyboard::Key::Divide;
        case GLFW_KEY_LEFT:          return re::Keyboard::Key::Left;
        case GLFW_KEY_RIGHT:         return re::Keyboard::Key::Right;
        case GLFW_KEY_UP:            return re::Keyboard::Key::Up;
        case GLFW_KEY_DOWN:          return re::Keyboard::Key::Down;
        case GLFW_KEY_KP_0:          return re::Keyboard::Key::Numpad0;
        case GLFW_KEY_KP_1:          return re::Keyboard::Key::Numpad1;
        case GLFW_KEY_KP_2:          return re::Keyboard::Key::Numpad2;
        case GLFW_KEY_KP_3:          return re::Keyboard::Key::Numpad3;
        case GLFW_KEY_KP_4:          return re::Keyboard::Key::Numpad4;
        case GLFW_KEY_KP_5:          return re::Keyboard::Key::Numpad5;
        case GLFW_KEY_KP_6:          return re::Keyboard::Key::Numpad6;
        case GLFW_KEY_KP_7:          return re::Keyboard::Key::Numpad7;
        case GLFW_KEY_KP_8:          return re::Keyboard::Key::Numpad8;
        case GLFW_KEY_KP_9:          return re::Keyboard::Key::Numpad9;
        case GLFW_KEY_F1:            return re::Keyboard::Key::F1;
        case GLFW_KEY_F2:            return re::Keyboard::Key::F2;
        case GLFW_KEY_F3:            return re::Keyboard::Key::F3;
        case GLFW_KEY_F4:            return re::Keyboard::Key::F4;
        case GLFW_KEY_F5:            return re::Keyboard::Key::F5;
        case GLFW_KEY_F6:            return re::Keyboard::Key::F6;
        case GLFW_KEY_F7:            return re::Keyboard::Key::F7;
        case GLFW_KEY_F8:            return re::Keyboard::Key::F8;
        case GLFW_KEY_F9:            return re::Keyboard::Key::F9;
        case GLFW_KEY_F10:           return re::Keyboard::Key::F10;
        case GLFW_KEY_F11:           return re::Keyboard::Key::F11;
        case GLFW_KEY_F12:           return re::Keyboard::Key::F12;
        case GLFW_KEY_F13:           return re::Keyboard::Key::F13;
        case GLFW_KEY_F14:           return re::Keyboard::Key::F14;
        case GLFW_KEY_F15:           return re::Keyboard::Key::F15;
        case GLFW_KEY_PAUSE:         return re::Keyboard::Key::Pause;
		default:                     return re::Keyboard::Key::Unknown;
    }
}

constexpr re::Mouse::Button GlfwMouseToButton(const int glfwButton)
{
	switch (glfwButton)
	{
	case GLFW_MOUSE_BUTTON_LEFT:   return re::Mouse::Button::Left;
	case GLFW_MOUSE_BUTTON_RIGHT:  return re::Mouse::Button::Right;
	case GLFW_MOUSE_BUTTON_MIDDLE: return re::Mouse::Button::Middle;
	case GLFW_MOUSE_BUTTON_4:      return re::Mouse::Button::Extra1;
	case GLFW_MOUSE_BUTTON_5:      return re::Mouse::Button::Extra2;
	default:                       return re::Mouse::Button::Left;
	}
}

} // namespace

namespace re::render
{

static bool s_glfwInitialized = false;

GLFWWindow::GLFWWindow(const std::string& title, const std::uint32_t width, const std::uint32_t height)
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

	m_window = glfwCreateWindow(
		static_cast<int>(m_data.width),
		static_cast<int>(m_data.height),
		title.c_str(),
		nullptr, nullptr);

	if (!m_window)
	{
		std::cerr << "Could not create GLFW window!" << std::endl;
		glfwTerminate();
		return;
	}

	// Делаем контекст активным
	glfwMakeContextCurrent(m_window);

	// Инициализируем GLAD (загружаем функции OpenGL)
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
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
	glfwSetKeyCallback(m_window, [](GLFWwindow* window, const int key, int, const int action, const int mods) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

		const auto engineKey = GlfwKeyToKey(key);
		const bool alt = (mods & GLFW_MOD_ALT);
		const bool ctrl = (mods & GLFW_MOD_CONTROL);
		const bool shift = (mods & GLFW_MOD_SHIFT);
		// system/super клавишу можно достать через GLFW_MOD_SUPER

		if (action == GLFW_PRESS)
		{
			data.eventQueue.emplace_back(Event::KeyPressed{ engineKey, alt, ctrl, shift });
		}
		else if (action == GLFW_RELEASE)
		{
			data.eventQueue.emplace_back(Event::KeyReleased{ engineKey, alt, ctrl, shift });
		}
		else if (action == GLFW_REPEAT)
		{
			data.eventQueue.emplace_back(Event::KeyPressed{ engineKey, alt, ctrl, shift });
		}
	});

	// 4. Text Entry (Unicode)
	glfwSetCharCallback(m_window, [](GLFWwindow* window, const unsigned int codepoint) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
		data.eventQueue.emplace_back(Event::TextEntered{ static_cast<char32_t>(codepoint) });
	});

	// 5. Mouse Button (Press, Release)
	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, const int button, const int action, int) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

		double x, y;
		glfwGetCursorPos(window, &x, &y);
		const Vector2i pos = { static_cast<int>(x), static_cast<int>(y) };

		const auto engineButton = GlfwMouseToButton(button);

		if (action == GLFW_PRESS)
		{
			data.eventQueue.emplace_back(Event::MouseButtonPressed{ engineButton, pos });
		}
		else if (action == GLFW_RELEASE)
		{
			data.eventQueue.emplace_back(Event::MouseButtonReleased{ engineButton, pos });
		}
	});

	// 6. Mouse Move
	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, const double x, const double y) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
		const Vector2i pos = { static_cast<int>(x), static_cast<int>(y) };
		data.eventQueue.emplace_back(Event::MouseMoved{ pos });
	});

	// 7. Scroll
	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double , const double yoffset) {
		WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

		double x, y;
		glfwGetCursorPos(window, &x, &y);
		const Vector2i pos = { static_cast<int>(x), static_cast<int>(y) };

		data.eventQueue.emplace_back(Event::MouseWheelScrolled{ static_cast<float>(yoffset), pos });
	});
}

void GLFWWindow::Shutdown()
{
	if (m_window)
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
}

std::optional<Event> GLFWWindow::PollEvent()
{
	glfwPollEvents();
	if (m_data.eventQueue.empty())
	{
		return std::nullopt;
	}

	Event e = m_data.eventQueue.front();
	m_data.eventQueue.erase(m_data.eventQueue.begin());
	return e;
}

Vector2u GLFWWindow::Size()
{
	return { m_data.width, m_data.height };
}

void GLFWWindow::SetVSyncEnabled(const bool enabled)
{
	if (m_window)
	{
		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(enabled);
		m_data.vsync = enabled;
	}
}

void* GLFWWindow::GetNativeHandle()
{
	return m_window;
}

bool GLFWWindow::IsOpen() const
{
	return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

bool GLFWWindow::SetActive(bool active)
{
	if (!m_window)
	{
		return false;
	}

	if (active)
	{
		glfwMakeContextCurrent(m_window);
	}
	else
	{
		glfwMakeContextCurrent(nullptr);
	}

	return true;
}

void GLFWWindow::SetTitle(const String& title)
{
	if (m_window)
	{
		m_data.title = title;
		glfwSetWindowTitle(m_window, title.ToString().c_str());
	}
}

void GLFWWindow::SetIcon(const Image& image)
{
	if (m_window && !image.IsEmpty())
	{
		GLFWimage icon[1];
		icon[0].width = static_cast<int>(image.Width());
		icon[0].height = static_cast<int>(image.Height());

		icon[0].pixels = const_cast<unsigned char*>(image.Data());
		glfwSetWindowIcon(m_window, 1, icon);
	}
}

Vector2f GLFWWindow::ToWorldPos(const Vector2i& pixelPos)
{
	if (m_worldPosCallback)
	{
		return m_worldPosCallback(pixelPos);
	}

	return Vector2f{ static_cast<float>(pixelPos.x), static_cast<float>(pixelPos.y) };
}

void GLFWWindow::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLFWWindow::Display()
{
	if (m_window)
	{
		glfwSwapBuffers(m_window);
	}
}

void GLFWWindow::SetWorldPosCallback(WorldPosCallback&& callback)
{
	m_worldPosCallback = std::move(callback);
}

} // namespace re::render