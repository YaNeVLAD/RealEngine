#pragma once

#include <Runtime/Export.hpp>

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include <cmath>

namespace re
{

class OrbitCameraSystem;

struct OrbitCameraComponent
{
	OrbitCameraComponent() = default;

	explicit OrbitCameraComponent(const ecs::Entity target)
		: target(target)
	{
	}

	ecs::Entity target = ecs::Entity::INVALID_ID;

	float distance = 25.0f;
	float pitch = -45.0f;
	float yaw = 0.0f;

	float minDistance = 5.0f;
	float maxDistance = 60.0f;
	Vector3f targetOffset = { 0.f, 0.f, 0.f };

private:
	bool m_isDragging = false;
	bool m_firstMouse = true;

	Vector2f m_lastMousePos = Vector2f(0.f);
	Vector2f m_delta = Vector2f(0.f);

	float m_deltaScroll = 0.0f;

	friend class OrbitCameraSystem;
};

struct OrbitCameraConfig
{
	Mouse::Button dragButton = Mouse::Button::Right;
	float sensitivityX = 0.3f;
	float sensitivityY = 0.3f;
	float zoomSpeed = 0.5f;
};

class RE_RUNTIME_API OrbitCameraSystem final : public ecs::System
{
public:
	explicit OrbitCameraSystem(const OrbitCameraConfig& config = {})
		: m_config(config)
	{
	}

	void Update(ecs::Scene& scene, core::TimeDelta) override
	{
		for (auto&& [entity, transform, orbit] : *scene.CreateView<TransformComponent, OrbitCameraComponent>())
		{
			orbit.yaw += orbit.m_delta.x * m_config.sensitivityX;
			orbit.pitch -= orbit.m_delta.y * m_config.sensitivityY;
			orbit.distance -= orbit.m_deltaScroll * m_config.zoomSpeed;

			orbit.m_delta = Vector2f(0.0f);
			orbit.m_deltaScroll = 0.0f;

			if (!scene.IsValid(orbit.target) || !scene.HasComponent<TransformComponent>(orbit.target))
			{
				continue;
			}

			orbit.pitch = std::clamp(orbit.pitch, -89.0f, 89.0f);
			orbit.distance = std::clamp(orbit.distance, orbit.minDistance, orbit.maxDistance);

			const auto& targetTransform = scene.GetComponent<TransformComponent>(orbit.target);

			const float pitchRad = std::cos(orbit.pitch * (std::numbers::pi_v<float> / 180.0f));
			const float sinPitch = std::sin(orbit.pitch * (std::numbers::pi_v<float> / 180.0f));
			const float yawRadCos = std::cos(orbit.yaw * (std::numbers::pi_v<float> / 180.0f));
			const float yawRadSin = std::sin(orbit.yaw * (std::numbers::pi_v<float> / 180.0f));

			const Vector3f offsetDir = { yawRadCos * pitchRad, sinPitch, yawRadSin * pitchRad };

			Vector3f targetPos = targetTransform.position + orbit.targetOffset;

			transform.position = targetPos - offsetDir * orbit.distance;

			transform.rotation.x = orbit.pitch;
			transform.rotation.y = orbit.yaw;
			transform.rotation.z = 0.0f;

			scene.MakeDirty<TransformComponent>(entity);
		}
	}

	void OnEvent(const Event& event, ecs::Scene& scene) const
	{
		if (const auto* mouseBtn = event.GetIf<Event::MouseButtonPressed>())
		{
			if (mouseBtn->button == m_config.dragButton)
			{
				for (auto&& [entity, orbit] : *scene.CreateView<OrbitCameraComponent>())
				{
					orbit.m_isDragging = true;
					orbit.m_firstMouse = true;
				}
			}
		}
		if (const auto* mouseBtn = event.GetIf<Event::MouseButtonReleased>())
		{
			if (mouseBtn->button == m_config.dragButton)
			{
				for (auto&& [entity, orbit] : *scene.CreateView<OrbitCameraComponent>())
				{
					orbit.m_isDragging = false;
				}
			}
		}

		if (const auto* mouseMoved = event.GetIf<Event::MouseMoved>())
		{
			const auto currentPos = static_cast<Vector2f>(mouseMoved->position);

			for (auto&& [entity, orbit] : *scene.CreateView<OrbitCameraComponent>())
			{
				if (!orbit.m_isDragging)
				{
					continue;
				}

				if (orbit.m_firstMouse)
				{
					orbit.m_lastMousePos = currentPos;
					orbit.m_firstMouse = false;
				}

				orbit.m_delta += (currentPos - orbit.m_lastMousePos);

				orbit.m_lastMousePos = currentPos;
			}
		}

		if (const auto* scroll = event.GetIf<Event::MouseWheelScrolled>())
		{
			for (auto&& [entity, orbit] : *scene.CreateView<OrbitCameraComponent>())
			{
				orbit.m_deltaScroll += static_cast<float>(scroll->delta);
			}
		}
	}

private:
	OrbitCameraConfig m_config;
};

} // namespace re