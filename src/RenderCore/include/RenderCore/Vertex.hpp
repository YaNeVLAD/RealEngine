#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>

namespace re
{

struct Vertex
{
	Vector3f position;
	Vector3f normal = { 0.f, 0.f, 1.f };
	Color color = Color::White;
	Vector2f texCoord = { 0.f, 0.f };
	float texIndex = -1.0f;
};

} // namespace re