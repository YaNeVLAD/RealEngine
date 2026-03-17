#pragma once

#include <Render3D/Export.hpp>

#include <Core/Math/Vector3.hpp>
#include <RenderCore/IRenderAPI.hpp>

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

	static void SetViewport(Vector2u size);

	static void SetDepthMask(bool enable);

	static void SetCullMode(CullMode mode);

private:
	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render