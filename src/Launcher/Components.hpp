#pragma once

#include <Core/Math/Vector2.hpp>

struct ChildComponent
{
	re::ecs::Entity parentEntity;
	re::Vector2f localPosition = { 0.f, 0.f };
	float localRotation = 0.0f;
};