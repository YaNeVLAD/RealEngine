#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>

#include "AsteroidsComponents.hpp"

class LifetimeSystem final : public re::ecs::System
{
public:
	void Update(re::ecs::Scene& scene, re::core::TimeDelta dt) override
	{
		UpdateLifetime<BulletComponent>(scene, dt);
		UpdateLifetime<ParticleComponent>(scene, dt);
	}

private:
	template <typename T>
	void UpdateLifetime(re::ecs::Scene& scene, re::core::TimeDelta dt)
	{
		for (auto&& [ent, comp] : *scene.CreateView<T>())
		{
			comp.lifetime -= dt;
			if (comp.lifetime <= 0.f)
			{
				scene.DestroyEntity(ent);
			}
		}
	}
};