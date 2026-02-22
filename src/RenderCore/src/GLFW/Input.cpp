#include <RenderCore/Internal/Input.hpp>

#include <GLFW/glfw3.h>

namespace
{

constexpr int KeyToGLFWKey(const re::Keyboard::Key key)
{
	// clang-format off
	switch (key)
    {
        case re::Keyboard::Key::A:          return GLFW_KEY_A;
        case re::Keyboard::Key::B:          return GLFW_KEY_B;
        case re::Keyboard::Key::C:          return GLFW_KEY_C;
        case re::Keyboard::Key::D:          return GLFW_KEY_D;
        case re::Keyboard::Key::E:          return GLFW_KEY_E;
        case re::Keyboard::Key::F:          return GLFW_KEY_F;
        case re::Keyboard::Key::G:          return GLFW_KEY_G;
        case re::Keyboard::Key::H:          return GLFW_KEY_H;
        case re::Keyboard::Key::I:          return GLFW_KEY_I;
        case re::Keyboard::Key::J:          return GLFW_KEY_J;
        case re::Keyboard::Key::K:          return GLFW_KEY_K;
        case re::Keyboard::Key::L:          return GLFW_KEY_L;
        case re::Keyboard::Key::M:          return GLFW_KEY_M;
        case re::Keyboard::Key::N:          return GLFW_KEY_N;
        case re::Keyboard::Key::O:          return GLFW_KEY_O;
        case re::Keyboard::Key::P:          return GLFW_KEY_P;
        case re::Keyboard::Key::Q:          return GLFW_KEY_Q;
        case re::Keyboard::Key::R:          return GLFW_KEY_R;
        case re::Keyboard::Key::S:          return GLFW_KEY_S;
        case re::Keyboard::Key::T:          return GLFW_KEY_T;
        case re::Keyboard::Key::U:          return GLFW_KEY_U;
        case re::Keyboard::Key::V:          return GLFW_KEY_V;
        case re::Keyboard::Key::W:          return GLFW_KEY_W;
        case re::Keyboard::Key::X:          return GLFW_KEY_X;
        case re::Keyboard::Key::Y:          return GLFW_KEY_Y;
        case re::Keyboard::Key::Z:          return GLFW_KEY_Z;
        case re::Keyboard::Key::Num0:       return GLFW_KEY_0;
        case re::Keyboard::Key::Num1:       return GLFW_KEY_1;
        case re::Keyboard::Key::Num2:       return GLFW_KEY_2;
        case re::Keyboard::Key::Num3:       return GLFW_KEY_3;
        case re::Keyboard::Key::Num4:       return GLFW_KEY_4;
        case re::Keyboard::Key::Num5:       return GLFW_KEY_5;
        case re::Keyboard::Key::Num6:       return GLFW_KEY_6;
        case re::Keyboard::Key::Num7:       return GLFW_KEY_7;
        case re::Keyboard::Key::Num8:       return GLFW_KEY_8;
        case re::Keyboard::Key::Num9:       return GLFW_KEY_9;
        case re::Keyboard::Key::Escape:     return GLFW_KEY_ESCAPE;
        case re::Keyboard::Key::LControl:   return GLFW_KEY_LEFT_CONTROL;
        case re::Keyboard::Key::LShift:     return GLFW_KEY_LEFT_SHIFT;
        case re::Keyboard::Key::LAlt:       return GLFW_KEY_LEFT_ALT;
        case re::Keyboard::Key::LSystem:    return GLFW_KEY_LEFT_SUPER;
        case re::Keyboard::Key::RControl:   return GLFW_KEY_RIGHT_CONTROL;
        case re::Keyboard::Key::RShift:     return GLFW_KEY_RIGHT_SHIFT;
        case re::Keyboard::Key::RAlt:       return GLFW_KEY_RIGHT_ALT;
        case re::Keyboard::Key::RSystem:    return GLFW_KEY_RIGHT_SUPER;
        case re::Keyboard::Key::Menu:       return GLFW_KEY_MENU;
        case re::Keyboard::Key::LBracket:   return GLFW_KEY_LEFT_BRACKET;
        case re::Keyboard::Key::RBracket:   return GLFW_KEY_RIGHT_BRACKET;
        case re::Keyboard::Key::Semicolon:  return GLFW_KEY_SEMICOLON;
        case re::Keyboard::Key::Comma:      return GLFW_KEY_COMMA;
        case re::Keyboard::Key::Period:     return GLFW_KEY_PERIOD;
        case re::Keyboard::Key::Apostrophe: return GLFW_KEY_APOSTROPHE;
        case re::Keyboard::Key::Slash:      return GLFW_KEY_SLASH;
        case re::Keyboard::Key::Backslash:  return GLFW_KEY_BACKSLASH;
        case re::Keyboard::Key::Grave:      return GLFW_KEY_GRAVE_ACCENT;
        case re::Keyboard::Key::Equal:      return GLFW_KEY_EQUAL;
        case re::Keyboard::Key::Hyphen:     return GLFW_KEY_MINUS;
        case re::Keyboard::Key::Space:      return GLFW_KEY_SPACE;
        case re::Keyboard::Key::Enter:      return GLFW_KEY_ENTER;
        case re::Keyboard::Key::Backspace:  return GLFW_KEY_BACKSPACE;
        case re::Keyboard::Key::Tab:        return GLFW_KEY_TAB;
        case re::Keyboard::Key::PageUp:     return GLFW_KEY_PAGE_UP;
        case re::Keyboard::Key::PageDown:   return GLFW_KEY_PAGE_DOWN;
        case re::Keyboard::Key::End:        return GLFW_KEY_END;
        case re::Keyboard::Key::Home:       return GLFW_KEY_HOME;
        case re::Keyboard::Key::Insert:     return GLFW_KEY_INSERT;
        case re::Keyboard::Key::Delete:     return GLFW_KEY_DELETE;
        case re::Keyboard::Key::Add:        return GLFW_KEY_KP_ADD;
        case re::Keyboard::Key::Subtract:   return GLFW_KEY_KP_SUBTRACT;
        case re::Keyboard::Key::Multiply:   return GLFW_KEY_KP_MULTIPLY;
        case re::Keyboard::Key::Divide:     return GLFW_KEY_KP_DIVIDE;
        case re::Keyboard::Key::Left:       return GLFW_KEY_LEFT;
        case re::Keyboard::Key::Right:      return GLFW_KEY_RIGHT;
        case re::Keyboard::Key::Up:         return GLFW_KEY_UP;
        case re::Keyboard::Key::Down:       return GLFW_KEY_DOWN;
        case re::Keyboard::Key::Numpad0:    return GLFW_KEY_KP_0;
        case re::Keyboard::Key::Numpad1:    return GLFW_KEY_KP_1;
        case re::Keyboard::Key::Numpad2:    return GLFW_KEY_KP_2;
        case re::Keyboard::Key::Numpad3:    return GLFW_KEY_KP_3;
        case re::Keyboard::Key::Numpad4:    return GLFW_KEY_KP_4;
        case re::Keyboard::Key::Numpad5:    return GLFW_KEY_KP_5;
        case re::Keyboard::Key::Numpad6:    return GLFW_KEY_KP_6;
        case re::Keyboard::Key::Numpad7:    return GLFW_KEY_KP_7;
        case re::Keyboard::Key::Numpad8:    return GLFW_KEY_KP_8;
        case re::Keyboard::Key::Numpad9:    return GLFW_KEY_KP_9;
        case re::Keyboard::Key::F1:         return GLFW_KEY_F1;
        case re::Keyboard::Key::F2:         return GLFW_KEY_F2;
        case re::Keyboard::Key::F3:         return GLFW_KEY_F3;
        case re::Keyboard::Key::F4:         return GLFW_KEY_F4;
        case re::Keyboard::Key::F5:         return GLFW_KEY_F5;
        case re::Keyboard::Key::F6:         return GLFW_KEY_F6;
        case re::Keyboard::Key::F7:         return GLFW_KEY_F7;
        case re::Keyboard::Key::F8:         return GLFW_KEY_F8;
        case re::Keyboard::Key::F9:         return GLFW_KEY_F9;
        case re::Keyboard::Key::F10:        return GLFW_KEY_F10;
        case re::Keyboard::Key::F11:        return GLFW_KEY_F11;
        case re::Keyboard::Key::F12:        return GLFW_KEY_F12;
        case re::Keyboard::Key::F13:        return GLFW_KEY_F13;
        case re::Keyboard::Key::F14:        return GLFW_KEY_F14;
        case re::Keyboard::Key::F15:        return GLFW_KEY_F15;
        case re::Keyboard::Key::Pause:      return GLFW_KEY_PAUSE;
        default:                            return GLFW_KEY_UNKNOWN;
    }
	// clang-format on
}

constexpr int MouseButtonToGLFWMouseButton(const re::Mouse::Button button)
{
	// clang-format off
	switch (button)
	{
		case re::Mouse::Button::Left:   return GLFW_MOUSE_BUTTON_LEFT;
		case re::Mouse::Button::Right:  return GLFW_MOUSE_BUTTON_RIGHT;
		case re::Mouse::Button::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
		case re::Mouse::Button::Extra1: return GLFW_MOUSE_BUTTON_4;
		case re::Mouse::Button::Extra2: return GLFW_MOUSE_BUTTON_5;
		default: return 0;
	}
	// clang-format on
}

} // namespace

namespace re::detail::Input
{

static GLFWwindow* s_windowHandle = nullptr;

void Init(void* nativeWindowHandle)
{
	s_windowHandle = static_cast<GLFWwindow*>(nativeWindowHandle);
}

bool Input::IsKeyPressed(const Keyboard::Key key)
{
	if (!s_windowHandle)
	{
		return false;
	}
	const int state = glfwGetKey(s_windowHandle, KeyToGLFWKey(key));

	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(const Mouse::Button button)
{
	if (!s_windowHandle)
	{
		return false;
	}
	const int state = glfwGetMouseButton(s_windowHandle, MouseButtonToGLFWMouseButton(button));

	return state == GLFW_PRESS;
}

Vector2i Input::GetMousePosition()
{
	if (!s_windowHandle)
	{
		return { 0, 0 };
	}

	double x, y;
	glfwGetCursorPos(s_windowHandle, &x, &y);

	return { static_cast<int>(x), static_cast<int>(y) };
}

} // namespace re::detail::Input