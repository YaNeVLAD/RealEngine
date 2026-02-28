#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>

#include <functional>

namespace re::detail
{

class RE_RUNTIME_API RenderSystem2D final : public ecs::System
{
public:
	explicit RenderSystem2D(render::IWindow& window);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	struct RenderObject
	{
		int zIndex;

		std::function<void()> drawCall;
	};

	render::IWindow& m_window;

	std::vector<RenderObject> m_renderQueue;
};

} // namespace re::detail