#include <Render2D/Renderer2D.hpp>

namespace re::render
{

void Renderer2D::Init(std::unique_ptr<IRenderAPI> renderApi)
{
	m_api = std::move(renderApi);
}

void Renderer2D::Shutdown()
{
}

void Renderer2D::BeginScene(core::Vector2f const& cameraPos, float cameraZoom)
{
	const core::Vector2f worldSize = {
		VIEWPORT_SIZE.x / cameraZoom,
		VIEWPORT_SIZE.y / cameraZoom
	};

	m_api->SetCamera(cameraPos, worldSize);
}

void Renderer2D::EndScene()
{
}

void Renderer2D::Clear()
{
	m_api->Clear();
}

void Renderer2D::DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color)
{
	m_api->DrawQuad(pos, size, color);
}

std::unique_ptr<IRenderAPI> Renderer2D::m_api = nullptr;

} // namespace re::render