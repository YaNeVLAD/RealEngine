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

	static void BeginScene(core::Vector2f const& cameraPos, float cameraZoom);

	static void EndScene();

	static void Clear();

	static void DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color);

private:
	static constexpr core::Vector2f VIEWPORT_SIZE = { 1920.f, 1080.f };

	static std::unique_ptr<IRenderAPI> m_api;
};

} // namespace re::render