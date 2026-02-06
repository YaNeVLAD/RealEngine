#include <RenderCore/Internal/Input.hpp>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace
{

constexpr sf::Keyboard::Key KeyToSfKey(const re::Keyboard::Key sfKey)
{
	// clang-format off
    switch (sfKey)
    {
	    case re::Keyboard::Key::A:          return sf::Keyboard::Key::A;
	    case re::Keyboard::Key::B:          return sf::Keyboard::Key::B;
	    case re::Keyboard::Key::C:          return sf::Keyboard::Key::C;
	    case re::Keyboard::Key::D:          return sf::Keyboard::Key::D;
	    case re::Keyboard::Key::E:          return sf::Keyboard::Key::E;
	    case re::Keyboard::Key::F:          return sf::Keyboard::Key::F;
	    case re::Keyboard::Key::G:          return sf::Keyboard::Key::G;
	    case re::Keyboard::Key::H:          return sf::Keyboard::Key::H;
	    case re::Keyboard::Key::I:          return sf::Keyboard::Key::I;
	    case re::Keyboard::Key::J:          return sf::Keyboard::Key::J;
	    case re::Keyboard::Key::K:          return sf::Keyboard::Key::K;
	    case re::Keyboard::Key::L:          return sf::Keyboard::Key::L;
	    case re::Keyboard::Key::M:          return sf::Keyboard::Key::M;
	    case re::Keyboard::Key::N:          return sf::Keyboard::Key::N;
	    case re::Keyboard::Key::O:          return sf::Keyboard::Key::O;
	    case re::Keyboard::Key::P:          return sf::Keyboard::Key::P;
	    case re::Keyboard::Key::Q:          return sf::Keyboard::Key::Q;
	    case re::Keyboard::Key::R:          return sf::Keyboard::Key::R;
	    case re::Keyboard::Key::S:          return sf::Keyboard::Key::S;
	    case re::Keyboard::Key::T:          return sf::Keyboard::Key::T;
	    case re::Keyboard::Key::U:          return sf::Keyboard::Key::U;
	    case re::Keyboard::Key::V:          return sf::Keyboard::Key::V;
	    case re::Keyboard::Key::W:          return sf::Keyboard::Key::W;
	    case re::Keyboard::Key::X:          return sf::Keyboard::Key::X;
	    case re::Keyboard::Key::Y:          return sf::Keyboard::Key::Y;
	    case re::Keyboard::Key::Z:          return sf::Keyboard::Key::Z;
	    case re::Keyboard::Key::Num0:       return sf::Keyboard::Key::Num0;
	    case re::Keyboard::Key::Num1:       return sf::Keyboard::Key::Num1;
	    case re::Keyboard::Key::Num2:       return sf::Keyboard::Key::Num2;
	    case re::Keyboard::Key::Num3:       return sf::Keyboard::Key::Num3;
	    case re::Keyboard::Key::Num4:       return sf::Keyboard::Key::Num4;
	    case re::Keyboard::Key::Num5:       return sf::Keyboard::Key::Num5;
	    case re::Keyboard::Key::Num6:       return sf::Keyboard::Key::Num6;
	    case re::Keyboard::Key::Num7:       return sf::Keyboard::Key::Num7;
	    case re::Keyboard::Key::Num8:       return sf::Keyboard::Key::Num8;
	    case re::Keyboard::Key::Num9:       return sf::Keyboard::Key::Num9;
	    case re::Keyboard::Key::Escape:     return sf::Keyboard::Key::Escape;
	    case re::Keyboard::Key::LControl:   return sf::Keyboard::Key::LControl;
	    case re::Keyboard::Key::LShift:     return sf::Keyboard::Key::LShift;
	    case re::Keyboard::Key::LAlt:       return sf::Keyboard::Key::LAlt;
	    case re::Keyboard::Key::LSystem:    return sf::Keyboard::Key::LSystem;
	    case re::Keyboard::Key::RControl:   return sf::Keyboard::Key::RControl;
	    case re::Keyboard::Key::RShift:     return sf::Keyboard::Key::RShift;
	    case re::Keyboard::Key::RAlt:       return sf::Keyboard::Key::RAlt;
	    case re::Keyboard::Key::RSystem:    return sf::Keyboard::Key::RSystem;
	    case re::Keyboard::Key::Menu:       return sf::Keyboard::Key::Menu;
	    case re::Keyboard::Key::LBracket:   return sf::Keyboard::Key::LBracket;
	    case re::Keyboard::Key::RBracket:   return sf::Keyboard::Key::RBracket;
	    case re::Keyboard::Key::Semicolon:  return sf::Keyboard::Key::Semicolon;
	    case re::Keyboard::Key::Comma:      return sf::Keyboard::Key::Comma;
	    case re::Keyboard::Key::Period:     return sf::Keyboard::Key::Period;
	    case re::Keyboard::Key::Apostrophe: return sf::Keyboard::Key::Apostrophe;
	    case re::Keyboard::Key::Slash:      return sf::Keyboard::Key::Slash;
	    case re::Keyboard::Key::Backslash:  return sf::Keyboard::Key::Backslash;
	    case re::Keyboard::Key::Grave:      return sf::Keyboard::Key::Grave;
	    case re::Keyboard::Key::Equal:      return sf::Keyboard::Key::Equal;
	    case re::Keyboard::Key::Hyphen:     return sf::Keyboard::Key::Hyphen;
	    case re::Keyboard::Key::Space:      return sf::Keyboard::Key::Space;
	    case re::Keyboard::Key::Enter:      return sf::Keyboard::Key::Enter;
	    case re::Keyboard::Key::Backspace:  return sf::Keyboard::Key::Backspace;
	    case re::Keyboard::Key::Tab:        return sf::Keyboard::Key::Tab;
	    case re::Keyboard::Key::PageUp:     return sf::Keyboard::Key::PageUp;
	    case re::Keyboard::Key::PageDown:   return sf::Keyboard::Key::PageDown;
	    case re::Keyboard::Key::End:        return sf::Keyboard::Key::End;
	    case re::Keyboard::Key::Home:       return sf::Keyboard::Key::Home;
	    case re::Keyboard::Key::Insert:     return sf::Keyboard::Key::Insert;
	    case re::Keyboard::Key::Delete:     return sf::Keyboard::Key::Delete;
	    case re::Keyboard::Key::Add:        return sf::Keyboard::Key::Add;
	    case re::Keyboard::Key::Subtract:   return sf::Keyboard::Key::Subtract;
	    case re::Keyboard::Key::Multiply:   return sf::Keyboard::Key::Multiply;
	    case re::Keyboard::Key::Divide:     return sf::Keyboard::Key::Divide;
	    case re::Keyboard::Key::Left:       return sf::Keyboard::Key::Left;
	    case re::Keyboard::Key::Right:      return sf::Keyboard::Key::Right;
	    case re::Keyboard::Key::Up:         return sf::Keyboard::Key::Up;
	    case re::Keyboard::Key::Down:       return sf::Keyboard::Key::Down;
	    case re::Keyboard::Key::Numpad0:    return sf::Keyboard::Key::Numpad0;
	    case re::Keyboard::Key::Numpad1:    return sf::Keyboard::Key::Numpad1;
	    case re::Keyboard::Key::Numpad2:    return sf::Keyboard::Key::Numpad2;
	    case re::Keyboard::Key::Numpad3:    return sf::Keyboard::Key::Numpad3;
	    case re::Keyboard::Key::Numpad4:    return sf::Keyboard::Key::Numpad4;
	    case re::Keyboard::Key::Numpad5:    return sf::Keyboard::Key::Numpad5;
	    case re::Keyboard::Key::Numpad6:    return sf::Keyboard::Key::Numpad6;
	    case re::Keyboard::Key::Numpad7:    return sf::Keyboard::Key::Numpad7;
	    case re::Keyboard::Key::Numpad8:    return sf::Keyboard::Key::Numpad8;
	    case re::Keyboard::Key::Numpad9:    return sf::Keyboard::Key::Numpad9;
	    case re::Keyboard::Key::F1:         return sf::Keyboard::Key::F1;
	    case re::Keyboard::Key::F2:         return sf::Keyboard::Key::F2;
	    case re::Keyboard::Key::F3:         return sf::Keyboard::Key::F3;
	    case re::Keyboard::Key::F4:         return sf::Keyboard::Key::F4;
	    case re::Keyboard::Key::F5:         return sf::Keyboard::Key::F5;
	    case re::Keyboard::Key::F6:         return sf::Keyboard::Key::F6;
	    case re::Keyboard::Key::F7:         return sf::Keyboard::Key::F7;
	    case re::Keyboard::Key::F8:         return sf::Keyboard::Key::F8;
	    case re::Keyboard::Key::F9:         return sf::Keyboard::Key::F9;
	    case re::Keyboard::Key::F10:        return sf::Keyboard::Key::F10;
	    case re::Keyboard::Key::F11:        return sf::Keyboard::Key::F11;
	    case re::Keyboard::Key::F12:        return sf::Keyboard::Key::F12;
	    case re::Keyboard::Key::F13:        return sf::Keyboard::Key::F13;
	    case re::Keyboard::Key::F14:        return sf::Keyboard::Key::F14;
	    case re::Keyboard::Key::F15:        return sf::Keyboard::Key::F15;
	    case re::Keyboard::Key::Pause:      return sf::Keyboard::Key::Pause;
	    default:							return sf::Keyboard::Key::Unknown;
    }
	// clang-format on
}

constexpr sf::Mouse::Button MouseButtonToSfMouseButton(const re::Mouse::Button button)
{
	// clang-format off
	switch (button)
	{
		case re::Mouse::Button::Left:   return sf::Mouse::Button::Left;
		case re::Mouse::Button::Right:  return sf::Mouse::Button::Right;
		case re::Mouse::Button::Middle: return sf::Mouse::Button::Middle;
		case re::Mouse::Button::Extra1: return sf::Mouse::Button::Extra1;
		case re::Mouse::Button::Extra2: return sf::Mouse::Button::Extra2;
	}
	// clang-format on

	return sf::Mouse::Button::Left;
}

} // namespace

namespace re::detail::Input
{

bool Input::IsKeyPressed(const Keyboard::Key key)
{
	return sf::Keyboard::isKeyPressed(KeyToSfKey(key));
}

bool Input::IsMouseButtonPressed(const Mouse::Button button)
{
	return sf::Mouse::isButtonPressed(MouseButtonToSfMouseButton(button));
}

Vector2i Input::GetMousePosition()
{
	const auto pos = sf::Mouse::getPosition();
	return { pos.x, pos.y };
}

} // namespace re::detail::Input