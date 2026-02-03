#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>

namespace re::runtime
{

struct TransformComponent
{
	core::Vector2f position = { 0.f, 0.f };
	core::Vector2f scale = { 1.f, 1.f };

	float rotation = 0.f;
};

struct SpriteComponent
{
	core::Color color = core::Color::White;
	core::Vector2f size = { 50.f, 50.f };
};

struct CameraComponent
{
	bool isPrimal = true;
	float zoom = 1.f;
};

} // namespace re::runtime