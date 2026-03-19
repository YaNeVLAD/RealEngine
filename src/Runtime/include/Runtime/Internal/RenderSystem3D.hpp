#pragma once

#include "RenderCore/GLFW/StaticMesh.hpp"

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>
#include <RenderCore/Vertex.hpp>

#include <glm/glm.hpp>

#include <map>

namespace re::detail
{

struct RenderCommand3D
{
	glm::mat4 transform{};

	StaticMesh* staticMesh = nullptr;

	const std::vector<Vertex>* vertices = nullptr;
	const std::vector<std::uint32_t>* indices = nullptr;

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

	std::map<std::pair<StaticMesh*, bool>, std::vector<glm::mat4>> m_instancedOpaque;
	std::map<std::pair<StaticMesh*, bool>, std::vector<std::pair<float, glm::mat4>>> m_instancedTransparent;
};

} // namespace re::detail