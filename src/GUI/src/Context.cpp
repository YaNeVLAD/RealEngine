#include <GUI/Context.hpp>

#include <RenderCore/Event.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>

#include <imgui.h>

namespace re::gui::Context
{

#if defined(RE_USE_FILAMENT_RENDER)

namespace
{

ImGuiKey ToImGuiKey(const Keyboard::Key key)
{
	switch (key)
	{ // clang-format off
	case Keyboard::Key::A:          return ImGuiKey_A;
	case Keyboard::Key::B:          return ImGuiKey_B;
	case Keyboard::Key::C:          return ImGuiKey_C;
	case Keyboard::Key::D:          return ImGuiKey_D;
	case Keyboard::Key::E:          return ImGuiKey_E;
	case Keyboard::Key::F:          return ImGuiKey_F;
	case Keyboard::Key::G:          return ImGuiKey_G;
	case Keyboard::Key::H:          return ImGuiKey_H;
	case Keyboard::Key::I:          return ImGuiKey_I;
	case Keyboard::Key::J:          return ImGuiKey_J;
	case Keyboard::Key::K:          return ImGuiKey_K;
	case Keyboard::Key::L:          return ImGuiKey_L;
	case Keyboard::Key::M:          return ImGuiKey_M;
	case Keyboard::Key::N:          return ImGuiKey_N;
	case Keyboard::Key::O:          return ImGuiKey_O;
	case Keyboard::Key::P:          return ImGuiKey_P;
	case Keyboard::Key::Q:          return ImGuiKey_Q;
	case Keyboard::Key::R:          return ImGuiKey_R;
	case Keyboard::Key::S:          return ImGuiKey_S;
	case Keyboard::Key::T:          return ImGuiKey_T;
	case Keyboard::Key::U:          return ImGuiKey_U;
	case Keyboard::Key::V:          return ImGuiKey_V;
	case Keyboard::Key::W:          return ImGuiKey_W;
	case Keyboard::Key::X:          return ImGuiKey_X;
	case Keyboard::Key::Y:          return ImGuiKey_Y;
	case Keyboard::Key::Z:          return ImGuiKey_Z;
	case Keyboard::Key::Num0:       return ImGuiKey_0;
	case Keyboard::Key::Num1:       return ImGuiKey_1;
	case Keyboard::Key::Num2:       return ImGuiKey_2;
	case Keyboard::Key::Num3:       return ImGuiKey_3;
	case Keyboard::Key::Num4:       return ImGuiKey_4;
	case Keyboard::Key::Num5:       return ImGuiKey_5;
	case Keyboard::Key::Num6:       return ImGuiKey_6;
	case Keyboard::Key::Num7:       return ImGuiKey_7;
	case Keyboard::Key::Num8:       return ImGuiKey_8;
	case Keyboard::Key::Num9:       return ImGuiKey_9;
	case Keyboard::Key::Escape:     return ImGuiKey_Escape;
	case Keyboard::Key::LControl:   return ImGuiKey_LeftCtrl;
	case Keyboard::Key::LShift:     return ImGuiKey_LeftShift;
	case Keyboard::Key::LAlt:       return ImGuiKey_LeftAlt;
	case Keyboard::Key::LSystem:    return ImGuiKey_LeftSuper;
	case Keyboard::Key::RControl:   return ImGuiKey_RightCtrl;
	case Keyboard::Key::RShift:     return ImGuiKey_RightShift;
	case Keyboard::Key::RAlt:       return ImGuiKey_RightAlt;
	case Keyboard::Key::RSystem:    return ImGuiKey_RightSuper;
	case Keyboard::Key::Menu:       return ImGuiKey_Menu;
	case Keyboard::Key::LBracket:   return ImGuiKey_LeftBracket;
	case Keyboard::Key::RBracket:   return ImGuiKey_RightBracket;
	case Keyboard::Key::Semicolon:  return ImGuiKey_Semicolon;
	case Keyboard::Key::Comma:      return ImGuiKey_Comma;
	case Keyboard::Key::Period:     return ImGuiKey_Period;
	case Keyboard::Key::Apostrophe: return ImGuiKey_Apostrophe;
	case Keyboard::Key::Slash:      return ImGuiKey_Slash;
	case Keyboard::Key::Backslash:  return ImGuiKey_Backslash;
	case Keyboard::Key::Grave:      return ImGuiKey_GraveAccent;
	case Keyboard::Key::Equal:      return ImGuiKey_Equal;
	case Keyboard::Key::Hyphen:     return ImGuiKey_Minus;
	case Keyboard::Key::Space:      return ImGuiKey_Space;
	case Keyboard::Key::Enter:      return ImGuiKey_Enter;
	case Keyboard::Key::Backspace:  return ImGuiKey_Backspace;
	case Keyboard::Key::Tab:        return ImGuiKey_Tab;
	case Keyboard::Key::PageUp:     return ImGuiKey_PageUp;
	case Keyboard::Key::PageDown:   return ImGuiKey_PageDown;
	case Keyboard::Key::End:        return ImGuiKey_End;
	case Keyboard::Key::Home:       return ImGuiKey_Home;
	case Keyboard::Key::Insert:     return ImGuiKey_Insert;
	case Keyboard::Key::Delete:     return ImGuiKey_Delete;
	case Keyboard::Key::Add:        return ImGuiKey_KeypadAdd;
	case Keyboard::Key::Subtract:   return ImGuiKey_KeypadSubtract;
	case Keyboard::Key::Multiply:   return ImGuiKey_KeypadMultiply;
	case Keyboard::Key::Divide:     return ImGuiKey_KeypadDivide;
	case Keyboard::Key::Left:       return ImGuiKey_LeftArrow;
	case Keyboard::Key::Right:      return ImGuiKey_RightArrow;
	case Keyboard::Key::Up:         return ImGuiKey_UpArrow;
	case Keyboard::Key::Down:       return ImGuiKey_DownArrow;
	case Keyboard::Key::Numpad0:    return ImGuiKey_Keypad0;
	case Keyboard::Key::Numpad1:    return ImGuiKey_Keypad1;
	case Keyboard::Key::Numpad2:    return ImGuiKey_Keypad2;
	case Keyboard::Key::Numpad3:    return ImGuiKey_Keypad3;
	case Keyboard::Key::Numpad4:    return ImGuiKey_Keypad4;
	case Keyboard::Key::Numpad5:    return ImGuiKey_Keypad5;
	case Keyboard::Key::Numpad6:    return ImGuiKey_Keypad6;
	case Keyboard::Key::Numpad7:    return ImGuiKey_Keypad7;
	case Keyboard::Key::Numpad8:    return ImGuiKey_Keypad8;
	case Keyboard::Key::Numpad9:    return ImGuiKey_Keypad9;
	case Keyboard::Key::F1:         return ImGuiKey_F1;
	case Keyboard::Key::F2:         return ImGuiKey_F2;
	case Keyboard::Key::F3:         return ImGuiKey_F3;
	case Keyboard::Key::F4:         return ImGuiKey_F4;
	case Keyboard::Key::F5:         return ImGuiKey_F5;
	case Keyboard::Key::F6:         return ImGuiKey_F6;
	case Keyboard::Key::F7:         return ImGuiKey_F7;
	case Keyboard::Key::F8:         return ImGuiKey_F8;
	case Keyboard::Key::F9:         return ImGuiKey_F9;
	case Keyboard::Key::F10:        return ImGuiKey_F10;
	case Keyboard::Key::F11:        return ImGuiKey_F11;
	case Keyboard::Key::F12:        return ImGuiKey_F12;
	case Keyboard::Key::F13:        return ImGuiKey_F13;
	case Keyboard::Key::F14:        return ImGuiKey_F14;
	case Keyboard::Key::F15:        return ImGuiKey_F15;
	case Keyboard::Key::Pause:      return ImGuiKey_Pause;
	default:                        return ImGuiKey_None;
	} // clang-format on
}

int ToImGuiMouseButton(const Mouse::Button button)
{
	switch (button)
	{ // clang-format off
	case Mouse::Button::Left:   return 0;
	case Mouse::Button::Right:  return 1;
	case Mouse::Button::Middle: return 2;
	case Mouse::Button::Extra1: return 3;
	case Mouse::Button::Extra2: return 4;
	default:                    return -1;
	} // clang-format on
}

void FeedInput(const Event& event)
{
	ImGuiIO& io = ImGui::GetIO();

	if (const auto* mouseMoved = event.GetIf<Event::MouseMoved>())
	{
		io.AddMousePosEvent(
			static_cast<float>(mouseMoved->position.x),
			static_cast<float>(mouseMoved->position.y));
	}
	else if (const auto* mousePressed = event.GetIf<Event::MouseButtonPressed>())
	{
		io.AddMouseButtonEvent(ToImGuiMouseButton(mousePressed->button), true);
	}
	else if (const auto* mouseReleased = event.GetIf<Event::MouseButtonReleased>())
	{
		io.AddMouseButtonEvent(ToImGuiMouseButton(mouseReleased->button), false);
	}
	else if (const auto* mouseScrolled = event.GetIf<Event::MouseWheelScrolled>())
	{
		io.AddMouseWheelEvent(0.0f, mouseScrolled->delta);
	}
	else if (const auto* keyPressed = event.GetIf<Event::KeyPressed>())
	{
		io.AddKeyEvent(ToImGuiKey(keyPressed->key), true);
	}
	else if (const auto* keyReleased = event.GetIf<Event::KeyReleased>())
	{
		io.AddKeyEvent(ToImGuiKey(keyReleased->key), false);
	}
	else if (const auto* textEntered = event.GetIf<Event::TextEntered>())
	{
		io.AddInputCharacter(static_cast<ImWchar>(textEntered->symbol));
	}
}

} // namespace

#endif

void Init([[maybe_unused]] void* nativeWindowHandle)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void Shutdown()
{
#if !defined(RE_USE_FILAMENT_RENDER)
	ImGui::DestroyContext();
#endif
}

void BeginFrame()
{
#if !defined(RE_USE_FILAMENT_RENDER)
	ImGui::NewFrame();
#endif
}

void EndFrame()
{
	ImGui::Render();
}

bool ProcessEvent(const Event& event)
{
#if defined(RE_USE_FILAMENT_RENDER)
	FeedInput(event);
#endif

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