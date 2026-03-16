#pragma once

#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Components.hpp>

#include "AsteroidsComponents.hpp"
#include "AsteroidsConstants.hpp"

class ShipControlSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		using namespace asteroids;

		for (auto&& [entity, transform, ship, vel] : *scene.CreateView<re::TransformComponent, ShipComponent, VelocityComponent>())
		{
			if (ship.respawnTimer > 0.f)
			{
				ship.respawnTimer -= dt;
				if (ship.respawnTimer <= 0.f)
				{
					ship.invulnerabilityTimer = ShipInvulnerabilityTimer;
					transform.position = { 0.f, 0.f, transform.position.z };
					transform.rotation = { 0.f, 0.f, 0.f };
					transform.scale = { 1.f, 1.f, 1.f };
					vel.linear = { 0.f, 0.f };
					vel.angular = 0.f;
				}
				continue;
			}

			if (ship.invulnerabilityTimer > 0.f)
			{
				ship.invulnerabilityTimer -= dt;
				if (static_cast<int>(ship.invulnerabilityTimer * 10.0f) % 2 == 0)
				{
					transform.scale = { 0.f, 0.f, 0.f };
				}
				else
				{
					transform.scale = { 1.f, 1.f, 1.f };
				}
			}
			else
			{
				transform.scale = { 1.f, 1.f, 1.f };
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Left) || re::Keyboard::IsKeyPressed(re::Keyboard::Key::A))
			{
				vel.angular -= ShipAngularAcceleration * dt;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Right) || re::Keyboard::IsKeyPressed(re::Keyboard::Key::D))
			{
				vel.angular += ShipAngularAcceleration * dt;
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Up) || re::Keyboard::IsKeyPressed(re::Keyboard::Key::W))
			{
				const re::Vector2f forward = re::Vector2f{ 0.f, -1.f }.Rotate(transform.rotation.z);
				vel.linear = vel.linear + forward * ShipLinearAcceleration * dt;
			}

			// Friction
			vel.linear -= vel.linear * ShipLinearFriction * dt;
			vel.angular -= vel.angular * ShipAngularFriction * dt;

			if (ship.fireCooldown > 0.f)
			{
				ship.fireCooldown -= dt;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Space)
				&& ship.fireCooldown <= 0.f
				&& ship.invulnerabilityTimer <= 0.f)
			{
				ship.fireCooldown = ShipFireCooldown;
				const re::Vector2f forward = re::Vector2f{ 0.f, -1.f }.Rotate(transform.rotation.z);
				const re::Vector2f bPos = {
					transform.position.x + forward.x * ShipBulletOffset,
					transform.position.y + forward.y * ShipBulletOffset
				};
				const re::Vector2f bVel = forward * BulletSpeed + vel.linear * 0.5f;

				scene.CreateEntity()
					.Add<re::TransformComponent>(re::Vector3f{ bPos.x, bPos.y, transform.position.z }, re::Vector3f{ 0.f, 0.f, transform.rotation.z })
					.Add<VelocityComponent>(bVel, 0.f)
					.Add<BulletComponent>()
					.Add<ScreenWrapComponent>(0.f)
					.Add<re::RectangleComponent>(re::Color::Yellow, re::Vector2f{ BulletWidth, BulletHeight });
			}
		}
	}
};