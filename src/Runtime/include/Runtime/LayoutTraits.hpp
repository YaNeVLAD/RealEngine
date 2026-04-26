#pragma once

#include <ECS/Scene.hpp>
#include <RenderCore/Event.hpp>
#include <Runtime/Layout.hpp>

#include <Runtime/System/MouseLookSystem.hpp>
#include <Runtime/System/OrbitCameraSystem.hpp>

namespace re
{

template <typename TDerived, typename... TTraits>
class LayoutTraits
	: public Layout
	, public TTraits...
{
public:
	using Layout::Layout;

	void OnEvent(Event const& event) final
	{
		(TTraits::ProcessTraitEvent(event, this->GetScene()), ...);

		static_cast<TDerived*>(this)->OnLayoutEvent(event);
	}

	virtual void OnLayoutEvent(Event const& event) {}
};

struct OrbitCameraTrait
{
	static void ProcessTraitEvent(const Event& event, ecs::Scene& scene)
	{
		if (scene.IsRegistered<OrbitCameraSystem>())
		{
			const auto& system = scene.GetSystem<OrbitCameraSystem>();

			system.OnEvent(event, scene);
		}
	}
};

struct MouseLookTrait
{
	static void ProcessTraitEvent(const Event& event, ecs::Scene& scene)
	{
		MouseLookSystem::OnEvent(event, scene);
	}
};

} // namespace re