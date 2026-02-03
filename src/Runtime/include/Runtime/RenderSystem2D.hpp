#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>

namespace re::runtime
{

class RE_RUNTIME_API RenderSystem2D final : public ecs::System
{
public:
	void Update(ecs::Scene& scene, core::TimeDelta dt) override;
};

} // namespace re::runtime