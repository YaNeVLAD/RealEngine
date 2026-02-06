#include <Render2D/Renderer2D.hpp>

namespace re::render
{

core::Vector2u Renderer2D::m_viewportSize = { 1920u, 1080u };

void Renderer2D::Init(std::unique_ptr<IRenderAPI> renderApi)
{
	m_api = std::move(renderApi);
}

void Renderer2D::Shutdown()
{
}

void Renderer2D::SetViewport(core::Vector2u const& newSize)
{
	m_viewportSize = newSize;
	m_api->SetViewport(
		{ 0.f, 0.f },
		{ static_cast<float>(newSize.x), static_cast<float>(newSize.y) });
}

void Renderer2D::BeginScene(core::Vector2f const& cameraPos, const float cameraZoom)
{
	const core::Vector2f worldSize = {
		m_viewportSize.x / cameraZoom,
		m_viewportSize.y / cameraZoom
	};

	m_api->SetCamera(cameraPos, worldSize);
}

void Renderer2D::EndScene()
{
	m_api->Flush();
}

void Renderer2D::DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color)
{
	m_api->DrawQuad(pos, size, color);
}

std::unique_ptr<IRenderAPI> Renderer2D::m_api = nullptr;

} // namespace re::render