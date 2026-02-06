#pragma once

#include <Render2D/Export.hpp>
#include <vector>

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <RenderCore/IRenderAPI.hpp>

namespace sf
{
class RenderWindow;
class Vertex;
} // namespace sf

namespace re::render
{

class RE_RENDER_2D_API SFMLRenderAPI final : public IRenderAPI
{
public:
	explicit SFMLRenderAPI(sf::RenderWindow& window);

	void SetViewport(Vector2f topLeft, Vector2f size) override;

	void SetCamera(Vector2f center, Vector2f size) override;

	void SetClearColor(Color const& color) override;

	void Flush() override;

	void DrawQuad(Vector2f const& pos, Vector2f const& size, float rotation, Color const& color) override;

	void DrawCircle(Vector2f const& center, float radius, Color const& color) override;

private:
	static constexpr std::size_t MAX_QUAD_COUNT = 1024;

	sf::RenderWindow& m_window;

	Color m_clearColor;

	std::vector<sf::Vertex> m_vertices;
};

} // namespace re::render