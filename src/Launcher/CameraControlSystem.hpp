#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Components.hpp>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

			transform.rotation.y -= camera.mouseDelta.x * sensitivity; // Yaw
			transform.rotation.x += camera.mouseDelta.y * sensitivity; // Pitch
			camera.mouseDelta = { 0.f, 0.f };

			transform.rotation.x = std::clamp(transform.rotation.x, -89.0f, 89.0f);

			glm::mat4 rotationMat(1.0f);
			rotationMat = glm::rotate(rotationMat, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
			rotationMat = glm::rotate(rotationMat, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
			rotationMat = glm::rotate(rotationMat, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));

			auto front = glm::vec3(rotationMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
			if (glm::length(front) > std::numeric_limits<float>::epsilon())
			{
				front = glm::normalize(front);
			}

			auto right = glm::cross(front, glm::vec3(0.f, 1.f, 0.f));
			if (glm::length(right) > std::numeric_limits<float>::epsilon())
			{
				right = glm::normalize(right);
			}

			camera.up = { 0.f, 1.f, 0.f };

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

			float movementSpeed = 10.0f;
			if (re::Keyboard::IsKeyPressed(re::Keyboard::Key::LShift))
			{
				movementSpeed *= 5.0f;
			}

			if (scene.HasComponent<re::RigidBodyComponent>(entity))
			{
				auto& rb = scene.GetComponent<re::RigidBodyComponent>(entity);
				rb.linearVelocity = { moveDir.x * movementSpeed, moveDir.y * movementSpeed, moveDir.z * movementSpeed };
				rb.isVelocityDirty = true;
			}
			else
			{
				const float moveStep = movementSpeed * dt;
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
			camera.fov = std::clamp(camera.fov, 1.0f, 120.0f);

			scene.MakeDirty<re::TransformComponent>(entity);
		}
	}
};