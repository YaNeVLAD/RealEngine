#pragma once

#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Components.hpp>

#include "AsteroidsComponents.hpp"

class ShipControlSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		for (auto&& [entity, transform, ship, vel] : *scene.CreateView<re::TransformComponent, ShipComponent, VelocityComponent>())
		{
			if (ship.respawnTimer > 0.f)
			{
				ship.respawnTimer -= dt;
				if (ship.respawnTimer <= 0.f)
				{
					ship.isInvulnerable = false;
					transform.position = { 0.f, 0.f };
					transform.rotation = 0.f;
					transform.scale = { 1.f, 1.f };
					vel.linear = { 0.f, 0.f };
					vel.angular = 0.f;
				}
				continue;
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Left))
			{
				vel.angular -= 1000.f * dt;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Right))
			{
				vel.angular += 1000.f * dt;
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Up))
			{
				const re::Vector2f forward = re::Vector2f{ 0.f, -1.f }.Rotate(transform.rotation);
				vel.linear = vel.linear + forward * 600.f * dt;
			}

			// Friction
			vel.linear -= vel.linear * 1.5f * dt;
			vel.angular -= vel.angular * 3.0f * dt;

			if (ship.fireCooldown > 0.f)
			{
				ship.fireCooldown -= dt;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Space) && ship.fireCooldown <= 0.f)
			{
				ship.fireCooldown = 0.25f;
				const re::Vector2f forward = re::Vector2f{ 0.f, -1.f }.Rotate(transform.rotation);
				const re::Vector2f bPos = transform.position + forward * 25.f;
				const re::Vector2f bVel = forward * 1000.f + vel.linear * 0.5f;

				scene.CreateEntity()
					.Add<re::TransformComponent>(bPos, transform.rotation)
					.Add<VelocityComponent>(bVel, 0.f)
					.Add<BulletComponent>()
					.Add<ScreenWrapComponent>(0.f)
					.Add<re::RectangleComponent>(re::Color::Yellow, re::Vector2f{ 5.f, 10.f });
			}
		}
	}
};