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

constexpr auto s_VertexShader3D = R"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;

    uniform mat4 u_ViewProjection;
    uniform mat4 u_Model;

    void main() {
        gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_FragmentShader3D = R"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    uniform vec4 u_Color;

    void main() {
        o_Color = u_Color;
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

constexpr GLenum ToOpenGLType(const re::PrimitiveType type)
{
	// clang-format off
	switch (type)
	{
	case re::PrimitiveType::Points:        return GL_POINTS;
	case re::PrimitiveType::Lines:         return GL_LINES;
	case re::PrimitiveType::LineStrip:     return GL_LINE_STRIP;
	case re::PrimitiveType::Triangles:     return GL_TRIANGLES;
	case re::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
	case re::PrimitiveType::TriangleFan:   return GL_TRIANGLE_FAN;
	default: return GL_POINTS;
	}
	// clang-format on
}

} // namespace

namespace re::render
{

void OpenGLRenderAPI::Init()
{
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_shaderProgram = CreateShader(s_VertexShaderSource, s_FragmentShaderSource);
	glUseProgram(m_shaderProgram);

	constexpr auto MaxQuads = 1000;
	constexpr auto MaxVertices = MaxQuads * 4;
	constexpr auto MaxIndices = MaxQuads * 6;

	glCreateVertexArrays(1, &m_vertexArray);
	glBindVertexArray(m_vertexArray);

	glCreateBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(InnerVertex), nullptr, GL_DYNAMIC_DRAW);

	// Pos(3 floats), Color(4 floats), TexCoord(2 floats), TexIndex(1 float)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT,
		GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, color)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, texCoord)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, texIndex)));

	// Pattern: 0,1,2, 2,3,0 ...
	auto* indices2d = new uint32_t[MaxIndices];
	uint32_t offset = 0;
	for (uint32_t i = 0; i < MaxIndices; i += 6)
	{
		indices2d[i + 0] = offset + 0;
		indices2d[i + 1] = offset + 1;
		indices2d[i + 2] = offset + 2;

		indices2d[i + 3] = offset + 2;
		indices2d[i + 4] = offset + 3;
		indices2d[i + 5] = offset + 0;

		offset += 4;
	}

	glCreateBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndices * sizeof(uint32_t), indices2d, GL_STATIC_DRAW);

	delete[] indices2d;

	// --- Init Dynamic Geometry ---
	glCreateVertexArrays(1, &m_dynamicVao);
	glBindVertexArray(m_dynamicVao);

	glCreateBuffers(1, &m_dynamicVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_dynamicVbo);

	glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(InnerVertex), nullptr, GL_DYNAMIC_DRAW);

	// Pos(3 floats), Color(4 floats), TexCoord(2 floats), TexIndex(1 float)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT,
		GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, color)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, texCoord)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1,
		GL_FLOAT, GL_FALSE, sizeof(InnerVertex),
		reinterpret_cast<const void*>(offsetof(InnerVertex, texIndex)));

	// 3D
	glEnable(GL_DEPTH_TEST);
	m_shader3D = CreateShader(s_VertexShader3D, s_FragmentShader3D);
	constexpr float vertices[] = {
		-0.5f, -0.5f, 0.5f, // 0: left-bottom-front
		0.5f, -0.5f, 0.5f, // 1: right-bottom-front
		0.5f, 0.5f, 0.5f, // 2: right-top-front
		-0.5f, 0.5f, 0.5f, // 3: left-top-front
		-0.5f, -0.5f, -0.5f, // 4: left-bottom-back
		0.5f, -0.5f, -0.5f, // 5: right-bottom-back
		0.5f, 0.5f, -0.5f, // 6: right-top-back
		-0.5f, 0.5f, -0.5f // 7: left-top-back
	};

	// Cube edges
	constexpr std::uint32_t indices[] = {
		0, 1, 2, 2, 3, 0, // Front
		1, 5, 6, 6, 2, 1, // Right
		5, 4, 7, 7, 6, 5, // Back
		4, 0, 3, 3, 7, 4, // Left
		3, 2, 6, 6, 7, 3, // Top
		4, 5, 1, 1, 0, 4 // Bottom
	};

	glGenVertexArrays(1, &m_cubeVao);
	glBindVertexArray(m_cubeVao);

	glGenBuffers(1, &m_cubeVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_cubeVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	glGenBuffers(1, &m_cubeEbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // DRAW LINES
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

	const GLint loc = glGetUniformLocation(m_shaderProgram, "u_ViewProjection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &m_viewProjection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		static_cast<long long>(m_batchBuffer.size() * sizeof(InnerVertex)),
		m_batchBuffer.data());

	glBindVertexArray(m_vertexArray);

	const GLsizei indexCount = static_cast<GLsizei>(m_batchBuffer.size() / 4) * 6;
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

	m_batchBuffer.clear();
}

void OpenGLRenderAPI::SetViewport(const Vector2f topLeft, const Vector2f size)
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

void OpenGLRenderAPI::DrawQuad(
	const Vector2f& pos,
	const Vector2f& size,
	const float rotation,
	const Color& color)
{
	DrawTexturedQuadImpl(pos, size, rotation, nullptr, color);
}

void OpenGLRenderAPI::DrawCircle(const Vector2f& center, float radius, const Color& color)
{
}

void OpenGLRenderAPI::DrawText(
	const String& text,
	const Font& font,
	const Vector2f& pos,
	float fontSize,
	const Color& color)
{
}

void OpenGLRenderAPI::DrawTexturedQuad(
	const Vector2f& pos,
	const Vector2f& size,
	Texture* texture,
	const Color& tint)
{
	DrawTexturedQuadImpl(pos, size, 0.0f, texture, tint);
}

void OpenGLRenderAPI::DrawMesh(std::vector<Vertex> const& vertices, PrimitiveType type)
{
	if (vertices.empty())
	{
		return;
	}

	Flush();

	glUseProgram(m_shaderProgram);

	const GLint loc = glGetUniformLocation(m_shaderProgram, "u_ViewProjection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, &m_viewProjection[0][0]);

	static std::vector<InnerVertex> glVertices;
	glVertices.clear();
	glVertices.reserve(vertices.size());

	for (const auto& [position, color, texCoords] : vertices)
	{
		const auto& [r, g, b, a] = color;
		const glm::vec2 uv(texCoords.x, texCoords.y);
		const glm::vec4 glColor = {
			static_cast<float>(r) / 255.0f,
			static_cast<float>(g) / 255.0f,
			static_cast<float>(b) / 255.0f,
			static_cast<float>(a) / 255.0f
		};

		glVertices.emplace_back(glm::vec3(position.x, position.y, 0.0f), glColor, uv, -1.0f);
	}

	const auto bufferSize = static_cast<GLsizeiptr>(glVertices.size() * sizeof(InnerVertex));
	glBindBuffer(GL_ARRAY_BUFFER, m_dynamicVbo);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, glVertices.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(m_dynamicVao);

	if (type == PrimitiveType::Points)
	{
		glPointSize(5.0f);
	}

	glDrawArrays(ToOpenGLType(type), 0, static_cast<GLsizei>(glVertices.size()));
}

void OpenGLRenderAPI::SetDepthTest(bool enabled)
{
	if (enabled)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void OpenGLRenderAPI::SetCameraPerspective(
	const float fov,
	const float aspectRatio,
	const float nearClip,
	const float farClip,
	const glm::mat4& viewMatrix)
{
	const glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
	m_viewProj3D = projection * viewMatrix;
}

void OpenGLRenderAPI::DrawCube(const glm::mat4& transform, const Color& color)
{
	glUseProgram(m_shader3D);

	// Загружаем матрицы
	const GLint vpLoc = glGetUniformLocation(m_shader3D, "u_ViewProjection");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &m_viewProj3D[0][0]);

	const GLint modelLoc = glGetUniformLocation(m_shader3D, "u_Model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &transform[0][0]);

	// Загружаем цвет
	const GLint colorLoc = glGetUniformLocation(m_shader3D, "u_Color");
	glUniform4f(colorLoc, (float)color.r / 255.f, (float)color.g / 255.f, (float)color.b / 255.f, (float)color.a / 255.f);

	// Рисуем
	glBindVertexArray(m_cubeVao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRenderAPI::DrawTexturedQuadImpl(
	Vector2f const& pos,
	Vector2f const& size,
	const float rotation,
	Texture* texture,
	Color const& color)
{
	const auto [r, g, b, a] = color;
	if (m_batchBuffer.size() >= 1000 * 4)
	{
		Flush();
	}

	float texIndex = -1.0f;
	if (texture)
	{
		Flush();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, *static_cast<uint32_t*>(texture->GetNativeHandle()));
		texIndex = 0.0f;
	}

	const glm::vec4 c = {
		static_cast<float>(r) / 255.0f,
		static_cast<float>(g) / 255.0f,
		static_cast<float>(b) / 255.0f,
		static_cast<float>(a) / 255.0f
	};

	float hw = size.x / 2.0f;
	float hh = size.y / 2.0f;

	glm::vec2 p[4] = {
		{ -hw, -hh },
		{ hw, -hh },
		{ hw, hh },
		{ -hw, hh }
	};

	if (rotation != 0.0f)
	{
		const float rad = glm::radians(rotation);
		const float s = std::sin(rad);
		const float c_rot = std::cos(rad);

		for (auto& i : p)
		{
			float x = i.x * c_rot - i.y * s;
			float y = i.x * s + i.y * c_rot;
			i = { x, y };
		}
	}

	m_batchBuffer.push_back({ { pos.x + p[0].x, pos.y + p[0].y, 0.0f }, c, { 0.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[1].x, pos.y + p[1].y, 0.0f }, c, { 1.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[2].x, pos.y + p[2].y, 0.0f }, c, { 1.0f, 1.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[3].x, pos.y + p[3].y, 0.0f }, c, { 0.0f, 1.0f }, texIndex });
}

} // namespace re::render