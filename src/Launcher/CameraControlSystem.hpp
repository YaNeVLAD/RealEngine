#pragma once

#include <ECS/Scene/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Components.hpp>

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

			const float moveSpeed = 10.0f * dt;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::W))
			{
				transform.position.x += front.x * moveSpeed;
				transform.position.y += front.y * moveSpeed;
				transform.position.z += front.z * moveSpeed;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::S))
			{
				transform.position.x -= front.x * moveSpeed;
				transform.position.y -= front.y * moveSpeed;
				transform.position.z -= front.z * moveSpeed;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::A))
			{
				transform.position.x -= right.x * moveSpeed;
				transform.position.y -= right.y * moveSpeed;
				transform.position.z -= right.z * moveSpeed;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::D))
			{
				transform.position.x += right.x * moveSpeed;
				transform.position.y += right.y * moveSpeed;
				transform.position.z += right.z * moveSpeed;
			}

			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::E))
			{
				transform.position.y += moveSpeed;
			}
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::Q))
			{
				transform.position.y -= moveSpeed;
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