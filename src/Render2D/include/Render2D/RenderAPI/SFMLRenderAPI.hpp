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

	void SetViewport(core::Vector2f topLeft, core::Vector2f size) override;

	void SetCamera(core::Vector2f center, core::Vector2f size) override;

	void SetClearColor(core::Color const& color) override;

	void Flush() override;

	void DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color) override;

private:
	static constexpr std::size_t MAX_QUAD_COUNT = 1024;

	sf::RenderWindow& m_window;

	core::Color m_clearColor;

	std::vector<sf::Vertex> m_vertices;
};

} // namespace re::render