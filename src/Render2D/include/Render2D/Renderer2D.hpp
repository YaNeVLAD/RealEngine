#pragma once

#include <Render2D/Export.hpp>

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <RenderCore/IRenderAPI.hpp>

#include <memory>

namespace re::render
{

class RE_RENDER_2D_API Renderer2D
{
public:
	static void Init(std::unique_ptr<IRenderAPI> renderApi);

	static void Shutdown();

	static void SetViewport(Vector2u const& newSize);

	static void BeginScene(Vector2f const& cameraPos, float cameraZoom);

	static void EndScene();

	static void DrawQuad(
		Vector2f const& pos,
		Vector2f const& size,
		float rotation,
		Color const& color);

	static void DrawCircle(
		Vector2f const& center,
		float radius,
		Color const& color);

private:
	static Vector2u m_viewportSize;

	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render