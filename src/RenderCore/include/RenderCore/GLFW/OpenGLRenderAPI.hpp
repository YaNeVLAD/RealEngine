#pragma once

#include <RenderCore/Export.hpp>
#include <RenderCore/IRenderAPI.hpp>
#include <RenderCore/Vertex.hpp>

#include <RenderCore/GLFW/Shader.hpp>
#include <RenderCore/GLFW/VertexArray.hpp>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace re::render
{

class RE_RENDER_CORE_API OpenGLRenderAPI final : public IRenderAPI
{
public:
	void Init() override;

	void ReloadShaders() override;

	void Clear() override;
	void Flush() override;
	void SetViewport(Vector2f topLeft, Vector2f size) override;
	Vector2f ScreenToWorld(Vector2i const& pixelPos) override;
	void SetCamera(Vector2f center, Vector2f size) override;

	// 2D
	void DrawQuad(const Vector3f& pos, const Vector2f& size, float rotation, const Color& color) override;
	void DrawCircle(const Vector3f& center, float radius, const Color& color) override;
	void DrawText(const String& text, const Font& font, const Vector3f& pos, float fontSize, const Color& color) override;
	void DrawTexturedQuad(const Vector3f& pos, const Vector2f& size, Texture* texture, const Color& tint) override;
	void DrawMesh(std::vector<Vertex> const& vertices, PrimitiveType type) override;

	// 3D
	void SetDepthTest(bool enabled) override;
	void SetDepthMask(bool writeEnabled) override;
	void SetCullMode(CullMode mode) override;
	void ResetCullingCache() override;
	void SetCameraPerspective(float fov, float aspectRatio, float nearClip, float farClip, const glm::mat4& viewMatrix) override;
	void DrawMesh3D(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform, bool wireframe) override;
	void DrawStaticMesh3D(StaticMesh* mesh, const glm::mat4& transform, bool wireframe) override;
	void DrawStaticMeshInstanced(StaticMesh* mesh, const std::vector<glm::mat4>& transforms, bool wireframe) override;
	void DrawStaticMeshGPUCulled(std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, float boundingRadius, const glm::vec3& cameraPos, bool wireframe, float farClip) override;
	void SetLight(const LightData& light) override;
	void SetMaterial(const Material& material) override;

	void DrawSkybox(std::uint32_t cubemapID, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;
	std::uint32_t CreateCubemapFromHDR(Texture* hdrTexture) override;
	std::uint32_t CreateIrradianceMap(std::uint32_t envCubemap) override;
	void SetEnvironment(std::uint32_t irradianceMap) override;

private:
	void BindStandard3DUniforms(const glm::mat4& transform) const;
	void DrawTexturedQuadImpl(const Vector3f& pos, const Vector2f& size, float rotation, Texture* texture, const Color& color);

private:
	std::shared_ptr<Shader> m_Shader2D;
	std::shared_ptr<VertexArray> m_BatchVAO;
	std::shared_ptr<VertexBuffer> m_BatchVBO;
	std::shared_ptr<IndexBuffer> m_BatchEBO;

	std::shared_ptr<VertexArray> m_DynamicVAO;
	std::shared_ptr<VertexBuffer> m_DynamicVBO;

	std::shared_ptr<Shader> m_Shader3D;
	std::shared_ptr<VertexArray> m_DynamicVAO3D;
	std::shared_ptr<VertexBuffer> m_DynamicVBO3D;
	std::shared_ptr<IndexBuffer> m_DynamicEBO3D;

	std::uint32_t m_normalVBOId = 0;

	std::shared_ptr<Shader> m_InstancedShader3D;
	std::uint32_t m_instanceVBOId = 0;

	std::shared_ptr<Shader> m_CullingComputeShader;
	struct CullingData
	{
		std::uint32_t inputSsbo = 0;
		std::uint32_t outputSsbo = 0;
		std::uint32_t cmdBuffer = 0;
		std::uint32_t normalSsbo = 0;
	};

	std::vector<CullingData> m_cullingBatches;

	std::shared_ptr<Shader> m_Shader3DPBR;
	std::shared_ptr<Shader> m_InstancedShader3DPBR;

	std::shared_ptr<VertexArray> m_CubeVAO3D;
	std::shared_ptr<VertexBuffer> m_CubeVBO3D;
	std::shared_ptr<Shader> m_SkyboxShader3D;

	std::shared_ptr<Shader> m_EquirectToCubeShader;
	std::shared_ptr<Shader> m_IrradianceShader;

	std::shared_ptr<Shader> m_AnimatedShader3DPBR;

	std::uint32_t m_boneSSBO = 0;

	LightData m_activeLight;

	std::vector<Vertex> m_batchBuffer{};

	glm::mat4 m_viewProjection{ 1.0f };
	glm::vec4 m_viewport{};
	glm::mat4 m_viewProj3D{ 1.0f };

	std::size_t m_dynamicOffset2D = 0;
	std::size_t m_dynamicOffsetVbo3D = 0;
	std::size_t m_dynamicOffsetEbo3D = 0;
};

} // namespace re::render