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
	    case re::Keyboard::Key::K:          return 0;
	    case re::Keyboard::Key::L:          return 0;
	    case re::Keyboard::Key::M:          return 0;
	    case re::Keyboard::Key::N:          return 0;
	    case re::Keyboard::Key::O:          return 0;
	    case re::Keyboard::Key::P:          return 0;
	    case re::Keyboard::Key::Q:          return 0;
	    case re::Keyboard::Key::R:          return 0;
	    case re::Keyboard::Key::S:          return 0;
	    case re::Keyboard::Key::T:          return 0;
	    case re::Keyboard::Key::U:          return 0;
	    case re::Keyboard::Key::V:          return 0;
	    case re::Keyboard::Key::W:          return 0;
	    case re::Keyboard::Key::X:          return 0;
	    case re::Keyboard::Key::Y:          return 0;
	    case re::Keyboard::Key::Z:          return 0;
	    case re::Keyboard::Key::Num0:       return 0;
	    case re::Keyboard::Key::Num1:       return 0;
	    case re::Keyboard::Key::Num2:       return 0;
	    case re::Keyboard::Key::Num3:       return 0;
	    case re::Keyboard::Key::Num4:       return 0;
	    case re::Keyboard::Key::Num5:       return 0;
	    case re::Keyboard::Key::Num6:       return 0;
	    case re::Keyboard::Key::Num7:       return 0;
	    case re::Keyboard::Key::Num8:       return 0;
	    case re::Keyboard::Key::Num9:       return 0;
	    case re::Keyboard::Key::Escape:     return 0;
	    case re::Keyboard::Key::LControl:   return 0;
	    case re::Keyboard::Key::LShift:     return 0;
	    case re::Keyboard::Key::LAlt:       return 0;
	    case re::Keyboard::Key::LSystem:    return 0;
	    case re::Keyboard::Key::RControl:   return 0;
	    case re::Keyboard::Key::RShift:     return 0;
	    case re::Keyboard::Key::RAlt:       return 0;
	    case re::Keyboard::Key::RSystem:    return 0;
	    case re::Keyboard::Key::Menu:       return 0;
	    case re::Keyboard::Key::LBracket:   return 0;
	    case re::Keyboard::Key::RBracket:   return 0;
	    case re::Keyboard::Key::Semicolon:  return 0;
	    case re::Keyboard::Key::Comma:      return 0;
	    case re::Keyboard::Key::Period:     return 0;
	    case re::Keyboard::Key::Apostrophe: return 0;
	    case re::Keyboard::Key::Slash:      return 0;
	    case re::Keyboard::Key::Backslash:  return 0;
	    case re::Keyboard::Key::Grave:      return 0;
	    case re::Keyboard::Key::Equal:      return 0;
	    case re::Keyboard::Key::Hyphen:     return 0;
	    case re::Keyboard::Key::Space:      return 0;
	    case re::Keyboard::Key::Enter:      return 0;
	    case re::Keyboard::Key::Backspace:  return 0;
	    case re::Keyboard::Key::Tab:        return 0;
	    case re::Keyboard::Key::PageUp:     return 0;
	    case re::Keyboard::Key::PageDown:   return 0;;
	    case re::Keyboard::Key::End:        return 0;
	    case re::Keyboard::Key::Home:       return 0;
	    case re::Keyboard::Key::Insert:     return 0;
	    case re::Keyboard::Key::Delete:     return 0;
	    case re::Keyboard::Key::Add:        return 0;
	    case re::Keyboard::Key::Subtract:   return 0;;
	    case re::Keyboard::Key::Multiply:   return 0;;
	    case re::Keyboard::Key::Divide:     return 0;
	    case re::Keyboard::Key::Left:       return 0;
	    case re::Keyboard::Key::Right:      return 0;
	    case re::Keyboard::Key::Up:         return 0;
	    case re::Keyboard::Key::Down:       return 0;
	    case re::Keyboard::Key::Numpad0:    return 0;
	    case re::Keyboard::Key::Numpad1:    return 0;
	    case re::Keyboard::Key::Numpad2:    return 0;
	    case re::Keyboard::Key::Numpad3:    return 0;
	    case re::Keyboard::Key::Numpad4:    return 0;
	    case re::Keyboard::Key::Numpad5:    return 0;
	    case re::Keyboard::Key::Numpad6:    return 0;
	    case re::Keyboard::Key::Numpad7:    return 0;
	    case re::Keyboard::Key::Numpad8:    return 0;
	    case re::Keyboard::Key::Numpad9:    return 0;
	    case re::Keyboard::Key::F1:         return 0;
	    case re::Keyboard::Key::F2:         return 0;
	    case re::Keyboard::Key::F3:         return 0;
	    case re::Keyboard::Key::F4:         return 0;
	    case re::Keyboard::Key::F5:         return 0;
	    case re::Keyboard::Key::F6:         return 0;
	    case re::Keyboard::Key::F7:         return 0;
	    case re::Keyboard::Key::F8:         return 0;
	    case re::Keyboard::Key::F9:         return 0;
	    case re::Keyboard::Key::F10:        return 0;
	    case re::Keyboard::Key::F11:        return 0;
	    case re::Keyboard::Key::F12:        return 0;
	    case re::Keyboard::Key::F13:        return 0;
	    case re::Keyboard::Key::F14:        return 0;
	    case re::Keyboard::Key::F15:        return 0;
	    case re::Keyboard::Key::Pause:      return 0;
	    default:							return 0;
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