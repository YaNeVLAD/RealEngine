#include <Render2D/Renderer2D.hpp>

namespace re::render
{

Vector2u Renderer2D::m_viewportSize = { 1920u, 1080u };

void Renderer2D::Init(std::unique_ptr<IRenderAPI> renderApi)
{
	m_api = std::move(renderApi);
	m_api->Init();
}

void Renderer2D::Shutdown()
{
}

void Renderer2D::SetViewport(Vector2u const& newSize)
{
	m_viewportSize = newSize;
	m_api->SetViewport(
		{ 0.f, 0.f },
		{ static_cast<float>(newSize.x), static_cast<float>(newSize.y) });
}

void Renderer2D::BeginScene(Vector2f const& cameraPos, const float cameraZoom)
{
	const Vector2f worldSize = {
		static_cast<float>(m_viewportSize.x) / cameraZoom,
		static_cast<float>(m_viewportSize.y) / cameraZoom
	};

	m_api->SetCamera(cameraPos, worldSize);
}

Vector2f Renderer2D::ScreenToWorld(Vector2i const& pixelPos)
{
	return m_api->ScreenToWorld(pixelPos);
}

void Renderer2D::EndScene()
{
	m_api->Flush();
}

void Renderer2D::DrawQuad(
	Vector2f const& pos,
	Vector2f const& size,
	const float rotation,
	Color const& color)
{
	m_api->DrawQuad(pos, size, rotation, color);
}

void Renderer2D::DrawCircle(
	Vector2f const& center,
	const float radius,
	Color const& color)
{
	m_api->DrawCircle(center, radius, color);
}

void Renderer2D::DrawText(
	String const& text,
	Font const& font,
	const Vector2f pos,
	const float size,
	Color const& color)
{
	m_api->DrawText(text, font, pos, size, color);
}

void Renderer2D::DrawTexturedQuad(
	Vector2f const& pos,
	Vector2f const& size,
	Texture* texture,
	const Color tint)
{
	m_api->DrawTexturedQuad(pos, size, texture, tint);
}

std::unique_ptr<IRenderAPI> Renderer2D::m_api = nullptr;

} // namespace re::render