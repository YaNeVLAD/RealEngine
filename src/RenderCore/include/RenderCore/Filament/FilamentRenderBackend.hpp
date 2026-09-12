#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Interface/IRenderBackend.hpp>

namespace re::render
{

class RE_RENDER_CORE_API FilamentRenderBackend final : public IRenderBackend
{
public:
	FilamentRenderBackend();
	~FilamentRenderBackend() override;

	void Init(void* windowHandle, std::uint32_t width, std::uint32_t height) override;
	void Shutdown() override;

	void Resize(std::uint32_t width, std::uint32_t height) override;

	RenderEntityHandle CreateStaticMesh(const MeshDataView& mesh, const MaterialDataView& material) override;
	RenderEntityHandle CreateLight(const LightDataView& light) override;
	void DestroyEntity(RenderEntityHandle handle) override;

	void UpdateTransform(RenderEntityHandle handle, const glm::mat4& transformMatrix) override;
	void UpdateLight(RenderEntityHandle handle, const LightDataView& light) override;
	void UpdateCamera(const CameraDataView& camera) override;
	void SetSkybox(std::uint32_t cubemapID, std::uint32_t irradianceID) override;

	void SetClearColor(Color color) override;

	void RenderUI(float dt, std::function<void()> uiCallback) override;

	void BeginFrame() override;
	void RenderFrame() override;
	void EndFrame() override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace re::render