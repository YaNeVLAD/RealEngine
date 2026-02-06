#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Math/Vector2.hpp>

namespace re::Mouse
{

enum class Button
{
	Left,
	Right,
	Middle,
	Extra1,
	Extra2,
};

RE_RENDER_CORE_API bool IsButtonPressed(Button button);

RE_RENDER_CORE_API Vector2i GetPosition();

} // namespace re::Mouse