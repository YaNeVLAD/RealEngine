#pragma once

#include <Core/Math/Vector2.hpp>

struct GravityComponent
{
	re::Vector2f velocity = { 0.0f, 0.0f };
	float startY = 0.0f;
	float phaseTime = 0.0f;
	bool isGrounded = false;
};

struct ChildComponent
{
	re::ecs::Entity parentEntity;
	re::Vector2f localPosition = { 0.f, 0.f };
	float localRotation = 0.0f;
};