#include <RenderCore/Mouse.hpp>

#include <RenderCore/Internal/Input.hpp>

namespace re::Mouse
{

bool IsButtonPressed(const Button button)
{
	return detail::Input::IsMouseButtonPressed(button);
}

Vector2i GetPosition()
{
	return detail::Input::GetMousePosition();
}

} // namespace re::Mouse