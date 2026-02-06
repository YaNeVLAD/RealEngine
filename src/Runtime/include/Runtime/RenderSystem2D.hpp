#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>

namespace re::runtime
{

class RE_RUNTIME_API RenderSystem2D final : public ecs::System
{
public:
	explicit RenderSystem2D(render::IWindow& window);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	render::IWindow& m_window;
};

} // namespace re::runtime