#include <RenderCore/Keyboard.hpp>

#include <RenderCore/Internal/Input.hpp>

namespace re::Keyboard
{

bool IsKeyPressed(const Key key)
{
	return detail::Input::IsKeyPressed(key);
}

} // namespace re::Keyboard