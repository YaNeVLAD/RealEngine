#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Components.hpp>

#include <cmath>
#include <glm/glm.hpp>

class CameraControlSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, const re::core::TimeDelta dt) override
	{
		for (auto&& [entity, transform, camera] : *scene.CreateView<re::TransformComponent, re::CameraComponent>())
		{
			if (!camera.isPrimal)
			{
				continue;
			}

			constexpr float sensitivity = 0.1f;

			transform.rotation.y += camera.mouseDelta.x * sensitivity; // Yaw
			transform.rotation.x += camera.mouseDelta.y * sensitivity; // Pitch

			camera.mouseDelta = { 0.f, 0.f };

			if (transform.rotation.x > 89.0f)
			{
				transform.rotation.x = 89.0f;
			}
			if (transform.rotation.x < -89.0f)
			{
				transform.rotation.x = -89.0f;
			}

			glm::vec3 front;
			front.x = std::cos(glm::radians(transform.rotation.y)) * std::cos(glm::radians(transform.rotation.x));
			front.y = std::sin(glm::radians(transform.rotation.x));
			front.z = std::sin(glm::radians(transform.rotation.y)) * std::cos(glm::radians(transform.rotation.x));
			front = glm::normalize(front);

			const glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.f, 1.f, 0.f)));
			const glm::vec3 up = glm::normalize(glm::cross(right, front));
			camera.up = { up.x, up.y, up.z };

			glm::vec3 moveDir(0.0f);
			// clang-format off
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::W)) moveDir += front;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::S)) moveDir -= front;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::A)) moveDir -= right;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::D)) moveDir += right;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::E)) moveDir += glm::vec3(0.0f, 1.0f, 0.0f);
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Q)) moveDir -= glm::vec3(0.0f, 1.0f, 0.0f);
			// clang-format on

			if (glm::length(moveDir) > std::numeric_limits<float>::epsilon())
			{
				moveDir = glm::normalize(moveDir);
			}

			constexpr float MOVE_SPEED = 10.0f;

			if (scene.HasComponent<re::RigidBodyComponent>(entity))
			{
				auto& rb = scene.GetComponent<re::RigidBodyComponent>(entity);

				rb.linearVelocity = { moveDir.x * MOVE_SPEED, moveDir.y * MOVE_SPEED, moveDir.z * MOVE_SPEED };
				rb.isVelocityDirty = true;
			}
			else
			{
				const float moveStep = MOVE_SPEED * dt;
				transform.position.x += moveDir.x * moveStep;
				transform.position.y += moveDir.y * moveStep;
				transform.position.z += moveDir.z * moveStep;
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Z))
			{
				camera.fov -= 30.0f * dt;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::X))
			{
				camera.fov += 30.0f * dt;
			}
			if (camera.fov < 1.0f)
			{
				camera.fov = 1.0f;
			}
			if (camera.fov > 120.0f)
			{
				camera.fov = 120.0f;
			}

			scene.MakeDirty<re::TransformComponent>(entity);
		}
	}
};