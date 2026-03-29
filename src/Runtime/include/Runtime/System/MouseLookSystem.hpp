#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <RenderCore/Event.hpp>
#include <Runtime/Components.hpp>

#include <algorithm>

namespace re
{

class MouseLookSystem final : public ecs::System
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
			const float currentX = static_cast<float>(mouseMoved->position.x);
			const float currentY = static_cast<float>(mouseMoved->position.y);

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