#include <Render2D/OpenGL/OpenGLRenderAPI.hpp>

#include "glm/ext/matrix_clip_space.hpp"

namespace re::render
{

OpenGLRenderAPI::OpenGLRenderAPI()
{
}

OpenGLRenderAPI::~OpenGLRenderAPI()
{
}

void OpenGLRenderAPI::Init()
{
	constexpr auto MaxVertices = 1024;

	glCreateVertexArrays(1, &m_vertexArray);
	glBindVertexArray(m_vertexArray);

	glCreateBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	// Выделяем память под N вершин (динамически)
	glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

	// Настраиваем Layout (как шейдер читает структуру Vertex)
	// 0: Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0);
	// 1: Color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));
}

void OpenGLRenderAPI::SetClearColor(const Color& color)
{
}

void OpenGLRenderAPI::Clear()
{
}

void OpenGLRenderAPI::Flush()
{
	if (m_batchBuffer.empty())
	{
		return;
	}

	glUseProgram(m_shaderProgram);

	// Загружаем матрицу
	GLint loc = glGetUniformLocation(m_shaderProgram, "u_ViewProjection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &m_viewProjection[0][0]);

	// Заливаем данные в VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (long long)m_batchBuffer.size() * sizeof(Vertex), m_batchBuffer.data());

	// Рисуем
	glBindVertexArray(m_vertexArray);
	const auto m_indexCount = 0;
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

	m_batchBuffer.clear();
}

void OpenGLRenderAPI::SetViewport(Vector2f topLeft, Vector2f size)
{
}

void OpenGLRenderAPI::SetCamera(const Vector2f center, const Vector2f size)
{
	const float left = center.x - size.x * 0.5f;
	const float right = center.x + size.x * 0.5f;
	const float bottom = center.y + size.y * 0.5f;
	const float top = center.y - size.y * 0.5f;

	// Создаем ортогональную проекцию (как в 2D играх)
	// В GLM ось Y обычно вверх, но в SFML/2D играх часто Y вниз.
	// Зависит от твоих предпочтений. Для Y вниз: bottom > top.
	m_viewProjection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
}

void OpenGLRenderAPI::DrawQuad(const Vector2f& pos, const Vector2f& size, float rotation, const Color& color)
{
}

void OpenGLRenderAPI::DrawCircle(const Vector2f& center, float radius, const Color& color)
{
}

void OpenGLRenderAPI::DrawText(const String& text, const Font& font, const Vector2f& pos, float fontSize, const Color& color)
{
}

void OpenGLRenderAPI::DrawTexturedQuad(const Vector2f& pos, const Vector2f& size, Texture* texture, const Color& tint)
{
}

} // namespace re::render