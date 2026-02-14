#pragma once

#include "Core/Math/Color.hpp"
#include "Core/Math/Vector2.hpp"
#include "Runtime/Components.hpp"
#include <ECS/Scene/Scene.hpp>

#include "../../Components.hpp"
#include "JumpPhysicsSystem.hpp"

namespace detail
{

constexpr re::Vector2f START_POS = { -200.f, 0.f };
constexpr float LETTER_SPACING = 250.f;
constexpr re::Color COLOR_K = re::Color::Red;
constexpr re::Color COLOR_B1 = re::Color::Green;
constexpr re::Color COLOR_B2 = re::Color::Blue;

inline void AttachChild(
	re::ecs::Scene& scene,
	re::ecs::EntityWrapper<re::ecs::Scene> const& parent,
	re::Vector2f offset,
	float rotation,
	std::function<void(re::ecs::EntityWrapper<re::ecs::Scene>)>&& setupFn)
{
	const auto child = scene.CreateEntity()
						   .Add<re::TransformComponent>(re::Vector2f{ 0, 0 }, 0.f)
						   .Add<ChildComponent>(parent.GetEntity(), offset, rotation);
	setupFn(child);
}

} // namespace detail

// Поменять шрифт, сделать сложнее
// Букву K усложнить
struct LettersLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>();

		scene
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>();

		scene.BuildSystemGraph();
		CreateLettersScene(scene);
	}

private:
	static void CreateKLetter(re::ecs::Scene& scene)
	{
		const auto root = scene.CreateEntity()
							  .Add<re::TransformComponent>(detail::START_POS, 0.0f)
							  .Add<re::RectangleComponent>(detail::COLOR_K, re::Vector2f{ 50.0f, 200.0f })
							  .Add<GravityComponent>(re::Vector2f{ 0, 0 }, 0.f, 0.0f);

		detail::AttachChild(scene, root, { 40.f, -50.f }, 45.0f, [](auto e) {
			e.template Add<re::RectangleComponent>(detail::COLOR_K, re::Vector2f{ 30.0f, 120.0f });
		});

		detail::AttachChild(scene, root, { 40.f, 50.f }, -45.0f, [](auto e) {
			e.template Add<re::RectangleComponent>(detail::COLOR_K, re::Vector2f{ 30.0f, 120.0f });
		});
	}

	static void CreateBLetter(
		re::ecs::Scene& scene,
		const float offsetX,
		const float phase,
		re::Color color)
	{
		re::Vector2f pos = detail::START_POS + re::Vector2f{ offsetX, 0.0f };

		const auto root = scene.CreateEntity()
							  .Add<re::TransformComponent>(pos, 0.0f)
							  .Add<re::RectangleComponent>(color, re::Vector2f{ 50.0f, 200.0f })
							  .Add<GravityComponent>(re::Vector2f{ 0, 0 }, 0.f, phase);

		constexpr float TOP_RADIUS = 50.0f;
		constexpr float BOTTOM_RADIUS = 60.0f;
		constexpr float HOLE_FACTOR = 0.4f;

		detail::AttachChild(scene, root, { 30.f, -50.f }, 0.0f, [&](auto e) {
			e.template Add<re::CircleComponent>(color, TOP_RADIUS);
		});

		detail::AttachChild(scene, root, { 30.f, -50.f }, 0.0f, [&](auto e) {
			e.template Add<re::CircleComponent>(re::Color::Black, TOP_RADIUS * HOLE_FACTOR);
		});

		detail::AttachChild(scene, root, { 40.f, 40.f }, 0.0f, [&](auto e) {
			e.template Add<re::CircleComponent>(color, BOTTOM_RADIUS);
		});

		detail::AttachChild(scene, root, { 40.f, 40.f }, 0.0f, [&](auto e) {
			e.template Add<re::CircleComponent>(re::Color::Black, BOTTOM_RADIUS * HOLE_FACTOR);
		});
	}

	static void CreateLettersScene(re::ecs::Scene& scene)
	{
		CreateKLetter(scene);
		CreateBLetter(scene, detail::LETTER_SPACING, 0.3f, detail::COLOR_B1);
		CreateBLetter(scene, detail::LETTER_SPACING * 2.f, 0.45f, detail::COLOR_B2);
	}
};