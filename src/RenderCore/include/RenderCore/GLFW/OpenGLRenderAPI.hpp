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

	// 2D
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

	void DrawMesh(std::vector<Vertex> const& vertices, PrimitiveType type) override;

	// 3D
	void SetDepthTest(bool enabled) override;

	void SetCameraPerspective(float fov, float aspectRatio, float nearClip, float farClip, const glm::mat4& viewMatrix) override;

	void DrawCube(const glm::mat4& transform, const Color& color) override;

private:
	void DrawTexturedQuadImpl(const Vector2f& pos, const Vector2f& size, float rotation, Texture* texture, const Color& color);

private:
	// OpenGL Handles
	std::uint32_t m_vertexArray = 0; // VAO
	std::uint32_t m_vertexBuffer = 0; // VBO
	std::uint32_t m_indexBuffer = 0; // EBO
	std::uint32_t m_shaderProgram = 0;

	std::uint32_t m_dynamicVao = 0;
	std::uint32_t m_dynamicVbo = 0;

	std::uint32_t m_cubeVao = 0;
	std::uint32_t m_cubeVbo = 0;
	std::uint32_t m_cubeEbo = 0;
	std::uint32_t m_shader3D = 0;
	glm::mat4 m_viewProj3D = glm::mat4(1.0f);

	struct InnerVertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texCoord;
		float texIndex;
	};

	std::vector<InnerVertex> m_batchBuffer{};

	glm::mat4 m_viewProjection{};
	glm::vec4 m_viewport{};
};
} // namespace re::render