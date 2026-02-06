#pragma once

#include "Components.hpp"
#include "Core/Math/Color.hpp"
#include "Core/Math/Vector2.hpp"
#include "Runtime/Components.hpp"

#include <ECS/Scene/Scene.hpp>

namespace detail
{

constexpr re::Vector2f START_POS = { -200.f, 0.f };
constexpr float LETTER_SPACING = 250.f;
constexpr re::Color COLOR_K = re::Color::Red;
constexpr re::Color COLOR_B1 = re::Color::Green;
constexpr re::Color COLOR_B2 = re::Color::Blue;

} // namespace detail

inline void CreateKLetter(re::ecs::Scene& scene)
{
	using namespace re;

	scene.CreateEntity()
		.Add<TransformComponent>(detail::START_POS, 0.0f)
		.Add<RectangleComponent>(detail::COLOR_K, Vector2f{ 40.0f, 200.0f })
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, 0.0f);

	scene.CreateEntity()
		.Add<TransformComponent>(detail::START_POS + Vector2f{ 55.f, -50.f }, 45.0f)
		.Add<RectangleComponent>(detail::COLOR_K, Vector2f{ 40.0f, 120.0f })
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, 0.0f);

	scene.CreateEntity()
		.Add<TransformComponent>(detail::START_POS + Vector2f{ 55.f, 50.f }, -45.0f)
		.Add<RectangleComponent>(detail::COLOR_K, Vector2f{ 40.0f, 120.0f })
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, 0.0f);
}

inline void CreateB1Letter(re::ecs::Scene& scene)
{
	using namespace re;

	Vector2f pos = detail::START_POS + Vector2f{ detail::LETTER_SPACING, 0.0f };
	float phase = 0.2f;

	scene.CreateEntity()
		.Add<TransformComponent>(pos, 0.0f)
		.Add<RectangleComponent>(detail::COLOR_B1, Vector2f{ 40.0f, 200.0f })
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);

	scene.CreateEntity()
		.Add<TransformComponent>(pos + Vector2f{ 30.f, -50.f })
		.Add<CircleComponent>(detail::COLOR_B1, 45.0f)
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);

	scene.CreateEntity()
		.Add<TransformComponent>(pos + Vector2f{ 30.f, 50.f })
		.Add<CircleComponent>(detail::COLOR_B1, 55.0f)
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);
}

inline void CreateB2Letter(re::ecs::Scene& scene)
{
	using namespace re;

	Vector2f pos = detail::START_POS + Vector2f{ detail::LETTER_SPACING * 2.0f, 0.0f };
	float phase = 0.4f;

	scene.CreateEntity()
		.Add<TransformComponent>(pos, 0.0f)
		.Add<RectangleComponent>(detail::COLOR_B2, Vector2f{ 40.0f, 200.0f })
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);

	scene.CreateEntity()
		.Add<TransformComponent>(pos + Vector2f{ 30.f, -50.f })
		.Add<CircleComponent>(detail::COLOR_B2, 45.0f)
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);

	scene.CreateEntity()
		.Add<TransformComponent>(pos + Vector2f{ 30.f, 50.f })
		.Add<CircleComponent>(detail::COLOR_B2, 55.0f)
		.Add<GravityComponent>(Vector2f{ 0, 0 }, 0.f, phase);
}