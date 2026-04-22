#pragma once

#include "AnimatedModel.hpp"
#include "Animator.hpp"

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/String.hpp>
#include <RenderCore/Font.hpp>
#include <RenderCore/Light.hpp>
#include <RenderCore/Material.hpp>
#include <RenderCore/PrimitiveType.hpp>
#include <RenderCore/StaticMesh.hpp>
#include <RenderCore/Texture.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

#include <glm/glm.hpp>

namespace re::render
{

enum class CullMode
{
	None = 0,
	Front = 1,
	Back = 2,
};

class IRenderAPI
{
public:
	virtual ~IRenderAPI() = default;

	virtual void Init() = 0;

	virtual void ReloadShaders() = 0;

	// 2D
	virtual void SetViewport(Vector2f topLeft, Vector2f size) = 0;
	virtual void SetCamera(Vector2f center, Vector2f size) = 0;
	virtual Vector2f ScreenToWorld(Vector2i const& pixelPos) = 0;
	virtual void Flush() = 0;
	virtual void Clear() = 0;
	virtual void DrawQuad(const Vector3f& pos, const Vector2f& size, float rotation, const Color& color) = 0;
	virtual void DrawCircle(Vector3f const& center, float radius, Color const& color) = 0;
	virtual void DrawText(String const& text, Font const& font, Vector3f const& pos, float fontSize, Color const& color) = 0;
	virtual void DrawTexturedQuad(Vector3f const& pos, Vector2f const& size, Texture* texture, Color const& tint) = 0;
	virtual void DrawMesh(std::vector<Vertex> const& vertices, PrimitiveType type) = 0;

	// 3D
	virtual void SetDepthTest(bool enabled) = 0;
	virtual void SetDepthMask(bool writeEnabled) = 0;
	virtual void SetCullMode(CullMode mode) = 0;
	virtual void ResetCullingCache() = 0;
	virtual void SetCameraPerspective(float fov, float aspectRatio, float nearClip, float farClip, const glm::mat4& viewMatrix) = 0;
	virtual void DrawMesh3D(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const glm::mat4& transform, bool wireframe) = 0;
	virtual void DrawStaticMesh3D(StaticMesh* mesh, const glm::mat4& transform, bool wireframe) = 0;
	virtual void DrawStaticMeshInstanced(StaticMesh* mesh, const std::vector<glm::mat4>& transforms, bool wireframe) = 0;
	virtual void DrawStaticMeshGPUCulled(std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, float boundingRadius, const glm::vec3& cameraPos, bool wireframe, float farClip) = 0;
	virtual void DrawAnimatedModel(AnimatedModel* model, Animator* animator, const glm::mat4& mat, bool wireframe) = 0;
	virtual void SetLight(const LightData& light) = 0;
	virtual void SetMaterial(const Material& material) = 0;

	virtual void DrawSkybox(std::uint32_t cubemapID, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) = 0;
	virtual std::uint32_t CreateCubemapFromHDR(Texture* hdrTexture) = 0;
	virtual std::uint32_t CreateIrradianceMap(std::uint32_t envCubemap) = 0;
	virtual void SetEnvironment(std::uint32_t irradianceMap) = 0;
};

} // namespace re::render