#include <RenderCore/GLFW/OpenGLRenderAPI.hpp>

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace
{

constexpr auto s_VertexShaderSource = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec4 a_Color;
    layout(location = 2) in vec2 a_TexCoord;
	layout(location = 3) in float a_TexIndex;

    uniform mat4 u_ViewProjection;

    out vec4 v_Color;
    out vec2 v_TexCoord;
    out float v_TexIndex;

    void main() {
        v_Color = a_Color;
        v_TexCoord = a_TexCoord;
        v_TexIndex = a_TexIndex;
        gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_FragmentShaderSource = R"(
    #version 450 core
    layout(location = 0) out vec4 color;

    in vec4 v_Color;
    in vec2 v_TexCoord;
    in float v_TexIndex;

    uniform sampler2D u_Texture;

    void main() {
        if (v_TexIndex < 0.0) {
            color = v_Color;
        } else {
            color = texture(u_Texture, v_TexCoord) * v_Color;
        }
    }
)";

GLuint CreateShader(const char* vertexSrc, const char* fragmentSrc)
{
	auto Compile = [](const GLenum type, const char* src) -> GLuint {
		const GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::cerr << "Shader Compilation Error:\n"
					  << infoLog << std::endl;
		}
		return shader;
	};

	const GLuint vs = Compile(GL_VERTEX_SHADER, vertexSrc);
	const GLuint fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc);
	const GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

} // namespace

namespace re::render
{

void OpenGLRenderAPI::Init()
{
	// 1. Включаем блендинг (прозрачность)
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 2. Компилируем шейдер
	m_shaderProgram = CreateShader(s_VertexShaderSource, s_FragmentShaderSource);
	glUseProgram(m_shaderProgram);

	// 3. Инициализируем буферы
	constexpr auto MaxQuads = 1000;
	constexpr auto MaxVertices = MaxQuads * 4;
	constexpr auto MaxIndices = MaxQuads * 6;

	glCreateVertexArrays(1, &m_vertexArray);
	glBindVertexArray(m_vertexArray);

	glCreateBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

	// Настройка Layout:
	// Pos(3 floats), Color(4 floats), TexCoord(2 floats), TexIndex(1 float)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4,
		GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, color)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2,
		GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1,
		GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, texIndex)));

	// 4. Индексный буфер (Pattern: 0,1,2, 2,3,0 ...)
	auto* indices = new uint32_t[MaxIndices];
	uint32_t offset = 0;
	for (uint32_t i = 0; i < MaxIndices; i += 6)
	{
		indices[i + 0] = offset + 0;
		indices[i + 1] = offset + 1;
		indices[i + 2] = offset + 2;

		indices[i + 3] = offset + 2;
		indices[i + 4] = offset + 3;
		indices[i + 5] = offset + 0;

		offset += 4;
	}

	glCreateBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);

	delete[] indices;
}

void OpenGLRenderAPI::SetClearColor(const Color& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRenderAPI::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderAPI::Flush()
{
	if (m_batchBuffer.empty())
	{
		return;
	}

	glUseProgram(m_shaderProgram);

	// Загружаем матрицу камеры
	const GLint loc = glGetUniformLocation(m_shaderProgram, "u_ViewProjection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &m_viewProjection[0][0]);

	// Загружаем данные вершин
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		static_cast<long long>(m_batchBuffer.size() * sizeof(Vertex)),
		m_batchBuffer.data());

	// Рисуем
	glBindVertexArray(m_vertexArray);

	// Кол-во индексов = (кол-во вершин / 4) * 6
	const GLsizei indexCount = static_cast<GLsizei>(m_batchBuffer.size() / 4) * 6;
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

	m_batchBuffer.clear();
}

void OpenGLRenderAPI::SetViewport(Vector2f topLeft, Vector2f size)
{
	glViewport(
		static_cast<GLint>(topLeft.x),
		static_cast<GLint>(topLeft.y),
		static_cast<GLsizei>(size.x),
		static_cast<GLsizei>(size.y));

	m_viewport = glm::vec4(topLeft.x, topLeft.y, size.x, size.y);
}

Vector2f OpenGLRenderAPI::ScreenToWorld(Vector2i const& pixelPos)
{
	const float x = static_cast<float>(pixelPos.x) + 0.5f;
	const float y = static_cast<float>(pixelPos.y) + 0.5f;

	float ndcX = (x - m_viewport.x) / m_viewport.z;
	float ndcY = (y - m_viewport.y) / m_viewport.w;

	ndcX = ndcX * 2.0f - 1.0f;

	ndcY = 1.0f - ndcY * 2.0f;

	const glm::vec4 clipSpacePos = {
		(x / m_viewport.z) * 2.0f - 1.0f,
		1.0f - y / m_viewport.w * 2.0f,
		0.0f,
		1.0f
	};

	const glm::mat4 inverseViewProj = glm::inverse(m_viewProjection);

	const glm::vec4 worldPos = inverseViewProj * clipSpacePos;

	return Vector2f{ worldPos.x, worldPos.y };
}

void OpenGLRenderAPI::SetCamera(const Vector2f center, const Vector2f size)
{
	const float left = center.x - size.x * 0.5f;
	const float right = center.x + size.x * 0.5f;
	const float bottom = center.y + size.y * 0.5f;
	const float top = center.y - size.y * 0.5f;

	m_viewProjection = glm::ortho(left, right, bottom, top, -10.0f, 10.0f);
}

void OpenGLRenderAPI::DrawQuad(const Vector2f& pos, const Vector2f& size, float rotation, const Color& color)
{
	DrawTexturedQuadImpl(pos, size, rotation, nullptr, color);
}

void OpenGLRenderAPI::DrawCircle(const Vector2f& center, float radius, const Color& color)
{
}

void OpenGLRenderAPI::DrawText(const String& text, const Font& font, const Vector2f& pos, float fontSize, const Color& color)
{
}

void OpenGLRenderAPI::DrawTexturedQuad(const Vector2f& pos, const Vector2f& size, Texture* texture, const Color& tint)
{
	DrawTexturedQuadImpl(pos, size, 0.0f, texture, tint);
}

void OpenGLRenderAPI::DrawTexturedQuadImpl(
	Vector2f const& pos,
	Vector2f const& size,
	const float rotation,
	Texture* texture,
	Color const& color)
{
	// Простейшая проверка на переполнение буфера
	if (m_batchBuffer.size() >= 1000 * 4) // Limit
	{
		Flush();
	}

	// Если рисуем текстуру, нужно её забиндить.
	// Если текущий батч рисовал другую текстуру или без текстуры, надо Flush.
	// (Пока упрощенно: если есть текстура - всегда Flush перед рисованием для безопасности)
	float texIndex = -1.0f;
	if (texture)
	{
		Flush(); // Разрываем батч
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, *static_cast<uint32_t*>(texture->GetNativeHandle()));
		texIndex = 0.0f;
	}

	// --- Математика ---
	const glm::vec4 c = {
		color.r / 255.0f,
		color.g / 255.0f,
		color.b / 255.0f,
		color.a / 255.0f
	};

	float hw = size.x / 2.0f;
	float hh = size.y / 2.0f;

	// Локальные координаты (центр в 0,0)
	glm::vec2 p[4] = {
		{ -hw, -hh },
		{ hw, -hh },
		{ hw, hh },
		{ -hw, hh }
	};

	// Вращение
	if (rotation != 0.0f)
	{
		// Конвертируем градусы в радианы
		const float rad = glm::radians(rotation);
		const float s = sin(rad);
		const float c_rot = cos(rad);

		for (auto& i : p)
		{
			float x = i.x * c_rot - i.y * s;
			float y = i.x * s + i.y * c_rot;
			i = { x, y };
		}
	}

	// Сдвиг в мировую позицию + добавление в буфер
	// Порядок UV: (0,0), (1,0), (1,1), (0,1) для стандартного OpenGL (Y вверх)
	// Но если у тебя SetCamera настроена так, что Y вниз (top < bottom), то UV могут быть другими.
	// Обычно: 0,0 - левый верх.

	m_batchBuffer.push_back({ { pos.x + p[0].x, pos.y + p[0].y, 0.0f }, c, { 0.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[1].x, pos.y + p[1].y, 0.0f }, c, { 1.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[2].x, pos.y + p[2].y, 0.0f }, c, { 1.0f, 1.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[3].x, pos.y + p[3].y, 0.0f }, c, { 0.0f, 1.0f }, texIndex });
}

} // namespace re::render