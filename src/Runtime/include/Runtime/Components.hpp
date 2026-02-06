#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>

namespace re
{

struct TransformComponent
{
	Vector2f position = { 0.f, 0.f };
	float rotation = 0.f;
	Vector2f scale = { 1.f, 1.f };
};

struct CircleComponent
{
	Color color = Color::White;
	float radius = 50.f;
};

struct RectangleComponent
{
	Color color = Color::White;
	Vector2f size = { 50.f, 50.f };
};

struct CameraComponent
{
	float zoom = 1.f;
	bool isPrimal = true;
};

} // namespace re