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

void SFMLRenderAPI::SetViewport(const Vector2f topLeft, const Vector2f size)
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

void SFMLRenderAPI::SetCamera(Vector2f center, Vector2f size)
{
	sf::View view = m_window.getView();

	view.setCenter({ center.x, center.y });
	view.setSize({ size.x, size.y });

	m_window.setView(view);
}

void SFMLRenderAPI::SetClearColor(Color const& color)
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
void SFMLRenderAPI::DrawQuad(
	Vector2f const& pos,
	Vector2f const& size,
	const float rotation,
	Color const& color)
{
	sf::Color sfColor(color.ToInt());

	const float hw = size.x / 2.0f;
	const float hh = size.y / 2.0f;

	Vector2f p0 = { -hw, -hh };
	Vector2f p1 = { hw, -hh };
	Vector2f p2 = { hw, hh };
	Vector2f p3 = { -hw, hh };

	if (rotation != 0.0f)
	{
		p0 = p0.Rotate(rotation);
		p1 = p1.Rotate(rotation);
		p2 = p2.Rotate(rotation);
		p3 = p3.Rotate(rotation);
	}

	m_vertices.emplace_back(sf::Vector2f(pos.x + p0.x, pos.y + p0.y), sfColor);
	m_vertices.emplace_back(sf::Vector2f(pos.x + p1.x, pos.y + p1.y), sfColor);
	m_vertices.emplace_back(sf::Vector2f(pos.x + p3.x, pos.y + p3.y), sfColor);

	m_vertices.emplace_back(sf::Vector2f(pos.x + p1.x, pos.y + p1.y), sfColor);
	m_vertices.emplace_back(sf::Vector2f(pos.x + p2.x, pos.y + p2.y), sfColor);
	m_vertices.emplace_back(sf::Vector2f(pos.x + p3.x, pos.y + p3.y), sfColor);
}

void SFMLRenderAPI::DrawCircle(
	Vector2f const& center,
	const float radius,
	Color const& color)
{
	sf::CircleShape circle(radius);
	circle.setOrigin({ radius, radius });
	circle.setPosition({ center.x, center.y });
	circle.setFillColor(sf::Color(color.ToInt()));

	m_window.draw(circle);
}

void SFMLRenderAPI::DrawText(
	std::string const& text,
	Font const& font,
	Vector2f const& pos,
	const float fontSize,
	Color const& color)
{
	Flush();

	sf::Text sfText(font.GetSfFont(), text, static_cast<unsigned>(fontSize));
	sfText.setOrigin({ pos.x / 2, pos.y / 2 });
	sfText.setFillColor(sf::Color(color.ToInt()));
	sfText.setPosition({ pos.x, pos.y });

	m_window.draw(sfText);
}

void SFMLRenderAPI::DrawTexturedQuad(
	Vector2f const& pos,
	Vector2f const& size,
	Texture* texture,
	Color const& tint)
{
	Flush();

	sf::RectangleShape rect(sf::Vector2f(size.x, size.y));
	rect.setPosition({ pos.x, pos.y });
	rect.setFillColor(sf::Color(tint.ToInt()));

	if (texture)
	{
		const auto sfTexture = static_cast<sf::Texture*>(texture->GetNativeHandle());
		rect.setTexture(sfTexture);
	}

	m_window.draw(rect);
}

} // namespace re::render