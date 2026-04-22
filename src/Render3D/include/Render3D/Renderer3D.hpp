#pragma once

#include <Render3D/Export.hpp>

#include <Core/Math/Vector3.hpp>
#include <RenderCore/IRenderAPI.hpp>
#include <RenderCore/Light.hpp>
#include <RenderCore/Material.hpp>

#include <glm/mat4x4.hpp>

#include <memory>

namespace re::render
{

class RE_RENDER_3D_API Renderer3D
{
public:
	static void Init(std::unique_ptr<IRenderAPI> api);

	static void BeginScene(float fov, float aspect, float nearClip, float farClip, const glm::mat4& viewMatrix);

	static void EndScene();

	static void DrawMesh(
		const std::vector<Vertex>& vertices,
		const std::vector<std::uint32_t>& indices,
		const Vector3f& pos,
		const Vector3f& scale,
		const Vector3f& rotation,
		bool wireframe);

	static void DrawMesh(
		const std::vector<Vertex>& vertices,
		const std::vector<std::uint32_t>& indices,
		const glm::mat4& transform,
		bool wireframe);

	static void DrawStaticMesh(
		StaticMesh* mesh,
		const glm::mat4& transform,
		bool wireframe);

	static void DrawStaticMeshInstanced(
		StaticMesh* mesh,
		const std::vector<glm::mat4>& transforms,
		bool wireframe);

	static void DrawStaticMeshGPUCulled(
		std::uint32_t batchIndex,
		StaticMesh* mesh,
		const std::vector<glm::mat4>& transforms,
		float boundingRadius,
		const glm::vec3& cameraPos,
		bool wireframe,
		float farClip);

	static void DrawSkybox(
		std::uint32_t cubemapID,
		const glm::mat4& viewMatrix,
		const glm::mat4& projectionMatrix);

	static std::uint32_t CreateCubemapFromHDR(Texture* hdrTexture);

	static std::uint32_t CreateIrradianceMap(std::uint32_t cubemapID);

	static void SetEnvironment(std::uint32_t irradianceID);

	static void SetMaterial(const Material& material);

	static void SetLight(const LightData& light);

	static void ResetCullingCache();

	static void SetViewport(Vector2u size);

	static void SetDepthMask(bool enable);

	static void SetCullMode(CullMode mode);

	static void ReloadShaders();

private:
	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render