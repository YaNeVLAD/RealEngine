#pragma once

#include <Runtime/Export.hpp>

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Event.hpp>
#include <Runtime/Components.hpp>

#include <algorithm>

namespace re
{

class MouseLookSystem;

struct MouseLookComponent
{
	explicit MouseLookComponent(const float sensitivity = 0.1f)
		: sensitivity(sensitivity)
	{
	}

	float sensitivity = 0.1f;

private:
	static constexpr float MIN_PITCH = -89.0f;
	static constexpr float MAX_PITCH = 89.0f;

	bool firstMouse = true;
	float lastX = 0.0f;
	float lastY = 0.0f;
	float deltaX = 0.0f;
	float deltaY = 0.0f;

	friend class MouseLookSystem;
};

class RE_RUNTIME_API MouseLookSystem final : public ecs::System
{
public:
	void Update(ecs::Scene& scene, core::TimeDelta) override
	{
		for (auto&& [entity, transform, mouseLook] : *scene.CreateView<TransformComponent, MouseLookComponent>())
		{
			if (std::abs(mouseLook.deltaX) > 0.001f || std::abs(mouseLook.deltaY) > 0.001f)
			{
				transform.rotation.y += mouseLook.deltaX * mouseLook.sensitivity;
				transform.rotation.x += mouseLook.deltaY * mouseLook.sensitivity;
				transform.rotation.x = std::clamp(transform.rotation.x, -89.0f, 89.0f);

				transform.rotation.y = std::fmod(transform.rotation.y, 360.0f);
				if (transform.rotation.y < 0.0f)
				{
					transform.rotation.y += 360.0f;
				}

				mouseLook.deltaX = 0.0f;
				mouseLook.deltaY = 0.0f;

				scene.MakeDirty<TransformComponent>(entity);
			}
		}
	}

	static void OnEvent(const Event& event, ecs::Scene& scene)
	{
		if (const auto* mouseMoved = event.GetIf<Event::MouseMoved>())
		{
			const auto currentX = static_cast<float>(mouseMoved->position.x);
			const auto currentY = static_cast<float>(mouseMoved->position.y);

			for (auto&& [entity, mouseLook] : *scene.CreateView<MouseLookComponent>())
			{
				if (mouseLook.firstMouse)
				{
					mouseLook.lastX = currentX;
					mouseLook.lastY = currentY;
					mouseLook.firstMouse = false;
				}

				mouseLook.deltaX += (currentX - mouseLook.lastX);
				mouseLook.deltaY += (mouseLook.lastY - currentY); // Invert Y

				mouseLook.lastX = currentX;
				mouseLook.lastY = currentY;
			}
		}
	}
};

} // namespace re