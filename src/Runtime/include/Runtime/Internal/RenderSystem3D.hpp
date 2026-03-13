#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>

namespace re::detail
{

class RE_RUNTIME_API RenderSystem3D final : public ecs::System
{
public:
	explicit RenderSystem3D(render::IWindow& window);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	render::IWindow& m_window;
};

} // namespace re::detail