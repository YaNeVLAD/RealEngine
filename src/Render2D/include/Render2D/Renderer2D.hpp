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

	static void SetViewport(core::Vector2u const& newSize);

	static void BeginScene(core::Vector2f const& cameraPos, float cameraZoom);

	static void EndScene();

	static void DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color);

private:
	static core::Vector2u m_viewportSize;

	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render