#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/String.hpp>
#include <RenderCore/Font.hpp>
#include <RenderCore/PrimitiveType.hpp>
#include <RenderCore/Texture.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

#include <glm/glm.hpp>

namespace re::render
{

class IRenderAPI
{
public:
	virtual ~IRenderAPI() = default;

	virtual void Init() = 0;

	// 2D
	virtual void SetViewport(Vector2f topLeft, Vector2f size) = 0;
	virtual void SetCamera(Vector2f center, Vector2f size) = 0;
	virtual void SetClearColor(Color const& color) = 0;
	virtual Vector2f ScreenToWorld(Vector2i const& pixelPos) = 0;
	virtual void Flush() = 0;
	virtual void Clear() = 0;
	virtual void DrawQuad(Vector2f const& pos, Vector2f const& size, float rotation, Color const& color) = 0;
	virtual void DrawCircle(Vector2f const& center, float radius, Color const& color) = 0;
	virtual void DrawText(String const& text, Font const& font, Vector2f const& pos, float fontSize, Color const& color) = 0;
	virtual void DrawTexturedQuad(Vector2f const& pos, Vector2f const& size, Texture* texture, Color const& tint) = 0;
	virtual void DrawMesh(std::vector<Vertex> const& vertices, PrimitiveType type) = 0;

	// 3D
	virtual void SetDepthTest(bool enabled) = 0;
	virtual void SetCameraPerspective(float fov, float aspectRatio, float nearClip, float farClip, const glm::mat4& viewMatrix) = 0;
	virtual void DrawCube(const glm::mat4& transform, const Color& color) = 0;
};

} // namespace re::render