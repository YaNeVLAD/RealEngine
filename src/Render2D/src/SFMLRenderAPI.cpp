#include <Render2D/RenderAPI/SFMLRenderAPI.hpp>

#include <SFML/Graphics.hpp>

namespace re::render
{

constexpr std::size_t VERTICES_PER_QUAD = 6;

SFMLRenderAPI::SFMLRenderAPI(sf::RenderWindow& window)
	: m_window(window)
{
	m_vertices.reserve(MAX_QUAD_COUNT * VERTICES_PER_QUAD);
}

void SFMLRenderAPI::SetViewport(const core::Vector2f topLeft, const core::Vector2f size)
{
	sf::View view = m_window.getView();
	view.setViewport(sf::FloatRect(
		sf::Vector2f(
			topLeft.x / static_cast<float>(m_window.getSize().x),
			topLeft.y / static_cast<float>(m_window.getSize().y)),
		sf::Vector2f(
			size.x / static_cast<float>(m_window.getSize().x),
			size.y / static_cast<float>(m_window.getSize().y))));

	m_window.setView(view);
}

void SFMLRenderAPI::SetCamera(core::Vector2f center, core::Vector2f size)
{
	sf::View view = m_window.getView();

	view.setCenter({ center.x, center.y });
	view.setSize({ size.x, size.y });

	m_window.setView(view);
}

void SFMLRenderAPI::SetClearColor(core::Color const& color)
{
	m_clearColor = color;
}

void SFMLRenderAPI::Flush()
{
	if (m_vertices.empty())
	{
		return;
	}

	m_window.draw(m_vertices.data(), m_vertices.size(), sf::PrimitiveType::Triangles);

	m_vertices.clear();
}

void SFMLRenderAPI::DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color)
{
	sf::Color sfColor(color.ToInt());

	const float left = pos.x;
	const float top = pos.y;
	const float right = pos.x + size.x;
	const float bottom = pos.y + size.y;

	m_vertices.emplace_back(sf::Vector2f(left, top), sfColor);
	m_vertices.emplace_back(sf::Vector2f(right, top), sfColor);
	m_vertices.emplace_back(sf::Vector2f(left, bottom), sfColor);

	m_vertices.emplace_back(sf::Vector2f(right, top), sfColor);
	m_vertices.emplace_back(sf::Vector2f(right, bottom), sfColor);
	m_vertices.emplace_back(sf::Vector2f(left, bottom), sfColor);
}

} // namespace re::render