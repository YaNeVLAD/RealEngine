#pragma once

#include <ECS/System/System.hpp>
#include <RenderCore/Interface/IRenderBackend.hpp>

#include <unordered_map>

namespace re::render
{

class RenderSystem3D : public ecs::System
{
public:
	explicit RenderSystem3D(IRenderBackend& backend);
	~RenderSystem3D() override = default;

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	IRenderBackend& m_backend;

	std::unordered_map<ecs::Entity, EntityRenderHandles> m_renderHandles;

	void ProcessCameras(ecs::Scene& scene) const;
	void ProcessLights(ecs::Scene& scene);
	void ProcessSkybox(ecs::Scene& scene) const;
	void SyncRenderEntities(ecs::Scene& scene);
	void CleanupDestroyedEntities(const ecs::Scene& scene);
};

} // namespace re::render