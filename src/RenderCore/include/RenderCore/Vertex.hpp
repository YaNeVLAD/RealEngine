#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>

namespace re
{

struct Vertex
{
	Vector2f position;
	Color color = Color::White;
	Vector2f texCoords{};
};

} // namespace re