#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/IRenderAPI.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace re::render
{
class RE_RENDER_CORE_API OpenGLRenderAPI final : public IRenderAPI
{
public:
	void Init() override;

	void SetClearColor(const Color& color) override;

	void Clear() override;

	void Flush() override;

	void SetViewport(Vector2f topLeft, Vector2f size) override;

	Vector2f ScreenToWorld(Vector2i const& pixelPos) override;

	void SetCamera(Vector2f center, Vector2f size) override;

	void DrawQuad(const Vector2f& pos, const Vector2f& size, float rotation, const Color& color) override;

	void DrawCircle(const Vector2f& center, float radius, const Color& color) override;

	void DrawText(const String& text, const Font& font, const Vector2f& pos, float fontSize, const Color& color) override;

	void DrawTexturedQuad(const Vector2f& pos, const Vector2f& size, Texture* texture, const Color& tint) override;

private:
	void DrawTexturedQuadImpl(const Vector2f& pos, const Vector2f& size, float rotation, Texture* texture, const Color& color);

private:
	// OpenGL Handles
	std::uint32_t m_vertexArray = 0; // VAO
	std::uint32_t m_vertexBuffer = 0; // VBO
	std::uint32_t m_indexBuffer = 0; // EBO
	std::uint32_t m_shaderProgram = 0;

	struct Vertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texCoord;
		float texIndex;
	};

	std::vector<Vertex> m_batchBuffer{};

	glm::mat4 m_viewProjection{};
	glm::vec4 m_viewport{};
};
} // namespace re::render