#pragma once

#include <RenderCore/IRenderAPI.hpp>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

namespace re::render
{
class OpenGLRenderAPI final : public IRenderAPI
{
public:
	OpenGLRenderAPI();
	~OpenGLRenderAPI() override;

	void Init() override;
	void SetClearColor(const Color& color) override;
	void Clear() override;

	void Flush() override;

	void SetViewport(Vector2f topLeft, Vector2f size) override;
	void SetCamera(Vector2f center, Vector2f size) override;
	void DrawQuad(const Vector2f& pos, const Vector2f& size, float rotation, const Color& color) override;
	void DrawCircle(const Vector2f& center, float radius, const Color& color) override;
	void DrawText(const String& text, const Font& font, const Vector2f& pos, float fontSize, const Color& color) override;
	void DrawTexturedQuad(const Vector2f& pos, const Vector2f& size, Texture* texture, const Color& tint) override;

private:
	// OpenGL Handles
	GLuint m_vertexArray = 0; // VAO
	GLuint m_vertexBuffer = 0; // VBO
	GLuint m_indexBuffer = 0; // EBO
	GLuint m_shaderProgram = 0;

	struct Vertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texCoord;
	};

	std::vector<Vertex> m_batchBuffer;

	glm::mat4 m_viewProjection;
};
} // namespace re::render