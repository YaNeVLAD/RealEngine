#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>
#include <RenderCore/Vertex.hpp>

#include <glm/glm.hpp>

namespace re::detail
{

struct RenderCommand3D
{
	glm::mat4 transform{};
	const std::vector<Vertex>* vertices = nullptr;
	const std::vector<uint32_t>* indices = nullptr;

	float distanceToCamera = 0.0f;
	bool wireframe = false;
};

class RE_RUNTIME_API RenderSystem3D final : public ecs::System
{
public:
	explicit RenderSystem3D(render::IWindow& window);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	render::IWindow& m_window;

	std::vector<RenderCommand3D> m_opaqueQueue;
	std::vector<RenderCommand3D> m_transparentQueue;
};

} // namespace re::detail