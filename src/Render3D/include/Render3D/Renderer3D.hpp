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

	static void BeginScene(float fov, float aspect, const glm::mat4& viewMatrix);

	static void EndScene();

	static void DrawCube(
		Vector3f const& position,
		Vector3f const& rotation,
		Vector3f const& scale,
		Color const& color);

	static void SetViewport(Vector2u size);

private:
	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render