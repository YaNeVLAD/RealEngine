#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <RenderCore/Assets/Font.hpp>
#include <RenderCore/Assets/Texture.hpp>

namespace re::render
{

class IRenderAPI
{
public:
	virtual ~IRenderAPI() = default;

	virtual void SetViewport(Vector2f topLeft, Vector2f size) = 0;
	virtual void SetCamera(Vector2f center, Vector2f size) = 0;
	virtual void SetClearColor(Color const& color) = 0;
	virtual void Flush() = 0;
	virtual void DrawQuad(Vector2f const& pos, Vector2f const& size, float rotation, Color const& color) = 0;
	virtual void DrawCircle(Vector2f const& center, float radius, Color const& color) = 0;
	virtual void DrawText(std::string const& text, Font const& font, Vector2f const& pos, float fontSize, Color const& color) = 0;
	virtual void DrawTexturedQuad(Vector2f const& pos, Vector2f const& size, Texture* texture, Color const& tint) = 0;
};

} // namespace re::render