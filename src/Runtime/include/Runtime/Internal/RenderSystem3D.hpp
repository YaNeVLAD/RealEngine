#pragma once

#include "RenderCore/GLFW/StaticMesh.hpp"

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/IWindow.hpp>
#include <RenderCore/Vertex.hpp>

#include <glm/glm.hpp>

namespace re::detail
{

struct StaticBatchItem
{
	StaticMesh* mesh;
	bool wireframe;
	float distance;
	glm::mat4 transform;
};

struct OpaqueBatchCache
{
	StaticMesh* mesh;
	bool wireframe;
	std::vector<glm::mat4> transforms;
};

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
	void ProcessTransforms(ecs::Scene& scene);
	void RebuildStaticOpaqueBatches(ecs::Scene& scene);
	void CollectDynamicAndTransparent(ecs::Scene& scene, const glm::vec3& camPos);

	void RenderOpaqueGeometry(const glm::vec3& camPos);
	void RenderTransparentGeometry();

	void DrawFlatBatch(const std::vector<StaticBatchItem>& batch);
	static void ExecuteRenderCommand(const RenderCommand3D& cmd);

private:
	render::IWindow& m_window;

	std::vector<RenderCommand3D> m_opaqueQueue;
	std::vector<RenderCommand3D> m_transparentQueue;

	std::vector<StaticBatchItem> m_flatOpaqueBatch;
	std::vector<StaticBatchItem> m_flatTransparentBatch;
	std::vector<glm::mat4> m_instanceBuffer;

	std::vector<OpaqueBatchCache> m_cachedOpaqueBatches;
	bool m_isStaticDirty = true;
};

} // namespace re::detail
