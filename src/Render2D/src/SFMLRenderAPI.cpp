#include <Render2D/SFMLRenderAPI.hpp>

#include <SFML/Graphics.hpp>

namespace re::render
{

SFMLRenderAPI::SFMLRenderAPI(sf::RenderWindow& window)
	: m_window(window)
{
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

void SFMLRenderAPI::Clear()
{
	m_window.clear(sf::Color(m_clearColor.ToInt()));
}

void SFMLRenderAPI::DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color)
{
	sf::RectangleShape rect(sf::Vector2f(size.x, size.y));
	rect.setPosition(sf::Vector2f(pos.x, pos.y));
	rect.setFillColor(sf::Color(color.ToInt()));

	m_window.draw(rect);
}

} // namespace re::render