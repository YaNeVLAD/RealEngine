#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Math/Vector2.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>

namespace re::detail::Input
{

RE_RENDER_CORE_API bool IsKeyPressed(Keyboard::Key key);

RE_RENDER_CORE_API bool IsMouseButtonPressed(Mouse::Button button);

RE_RENDER_CORE_API Vector2i GetMousePosition();

} // namespace re::detail::Input