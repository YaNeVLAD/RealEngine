#include <RenderCore/GLFW/OpenGLRenderAPI.hpp>

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
// 0: Position, 1: Normal, 2: Color, 3: TexCoord, 4: TexIndex
constexpr auto s_VertexShaderSource = UR"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec4 a_Color;
    layout(location = 3) in vec2 a_TexCoord;
    layout(location = 4) in float a_TexIndex;

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

constexpr auto s_FragmentShaderSource = UR"(
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

constexpr auto s_VertexShader3D = UR"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec4 a_Color;
    layout(location = 3) in vec2 a_TexCoord;
    layout(location = 4) in float a_TexIndex;

    uniform mat4 u_ViewProjection;
    uniform mat4 u_Model;

    out vec4 v_Color;
    out vec2 v_TexCoord;
    out vec3 v_Normal;

    void main() {
        v_Color = a_Color;
        v_TexCoord = a_TexCoord;
        v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
        gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_FragmentShader3D = UR"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    in vec4 v_Color;
    in vec2 v_TexCoord;
    in vec3 v_Normal;

    uniform vec4 u_Color;

    void main() {
        o_Color = v_Color * u_Color;
    }
)";

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
	glEnable(GL_DEPTH_TEST);

	constexpr auto MaxQuads = 1000;
	constexpr auto MaxVertices = MaxQuads * 4;
	constexpr auto MaxIndices = MaxQuads * 6;

	m_batchBuffer.reserve(MaxVertices);

	m_Shader2D = std::make_shared<Shader>(s_VertexShaderSource, s_FragmentShaderSource);
	m_Shader3D = std::make_shared<Shader>(s_VertexShader3D, s_FragmentShader3D);

	BufferLayout unifiedLayout;
	unifiedLayout.Push<Vector3f>("a_Position");
	unifiedLayout.Push<Vector3f>("a_Normal");
	unifiedLayout.Push<Color>("a_Color", true);
	unifiedLayout.Push<Vector2f>("a_TexCoord");
	unifiedLayout.Push<float>("a_TexIndex");

	// 1. Batching
	m_BatchVAO = std::make_shared<VertexArray>();
	m_BatchVBO = std::make_shared<VertexBuffer>(MaxVertices * sizeof(Vertex));
	m_BatchVAO->AddVertexBuffer(m_BatchVBO, unifiedLayout);

	auto* indices2d = new std::uint32_t[MaxIndices];
	std::uint32_t offset = 0;
	for (std::uint32_t i = 0; i < MaxIndices; i += 6)
	{
		indices2d[i + 0] = offset + 0;
		indices2d[i + 1] = offset + 1;
		indices2d[i + 2] = offset + 2;
		indices2d[i + 3] = offset + 2;
		indices2d[i + 4] = offset + 3;
		indices2d[i + 5] = offset + 0;
		offset += 4;
	}
	m_BatchEBO = std::make_shared<IndexBuffer>(indices2d, MaxIndices);
	m_BatchVAO->SetIndexBuffer(m_BatchEBO);
	delete[] indices2d;

	// 2. Dynamic 2D
	m_DynamicVAO = std::make_shared<VertexArray>();
	m_DynamicVBO = std::make_shared<VertexBuffer>(4096 * sizeof(Vertex));
	m_DynamicVAO->AddVertexBuffer(m_DynamicVBO, unifiedLayout);

	// 3. Dynamic 3D
	m_DynamicVAO3D = std::make_shared<VertexArray>();
	m_DynamicVBO3D = std::make_shared<VertexBuffer>(10000 * sizeof(Vertex));
	m_DynamicVAO3D->AddVertexBuffer(m_DynamicVBO3D, unifiedLayout);

	m_DynamicEBO3D = std::make_shared<IndexBuffer>(30000);
	m_DynamicVAO3D->SetIndexBuffer(m_DynamicEBO3D);
}

void OpenGLRenderAPI::SetClearColor(const Color& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRenderAPI::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_dynamicOffset2D = 0;
	m_dynamicOffsetVbo3D = 0;
	m_dynamicOffsetEbo3D = 0;
	m_batchBuffer.clear();
}

void OpenGLRenderAPI::Flush()
{
	if (m_batchBuffer.empty())
	{
		return;
	}

	m_Shader2D->Bind();
	m_Shader2D->SetMat4("u_ViewProjection", m_viewProjection);

	m_BatchVAO->Bind();
	m_BatchVBO->SetData(m_batchBuffer.data(), m_batchBuffer.size() * sizeof(Vertex));

	const auto indexCount = static_cast<std::uint32_t>(m_batchBuffer.size() / 4) * 6;
	glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_INT, nullptr);

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

	m_viewProjection = glm::ortho(left, right, bottom, top, -100.0f, 1000.0f);
}

void OpenGLRenderAPI::DrawQuad(const Vector3f& pos, const Vector2f& size, const float rotation, const Color& color)
{
	DrawTexturedQuadImpl(pos, size, rotation, nullptr, color);
}

void OpenGLRenderAPI::DrawCircle(const Vector3f& center, const float radius, const Color& color)
{
	std::ignore = center, std::ignore = radius, std::ignore = color;
}

void OpenGLRenderAPI::DrawText(
	const String& text,
	const Font& font,
	const Vector3f& pos,
	const float fontSize,
	const Color& color)
{
	std::ignore = text, std::ignore = font, std::ignore = pos, std::ignore = fontSize, std::ignore = color;
}

void OpenGLRenderAPI::DrawTexturedQuad(const Vector3f& pos, const Vector2f& size, Texture* texture, const Color& tint)
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

	m_Shader2D->Bind();
	m_Shader2D->SetMat4("u_ViewProjection", m_viewProjection);

	const std::size_t bytesNeeded = vertices.size() * sizeof(Vertex);
	if (m_dynamicOffset2D + bytesNeeded > m_DynamicVBO->GetSize())
	{
		m_dynamicOffset2D = 0;
	}

	m_DynamicVAO->Bind();
	m_DynamicVBO->SetData(vertices.data(), bytesNeeded, m_dynamicOffset2D);

	if (type == PrimitiveType::Points)
	{
		glPointSize(5.0f);
	}

	const auto firstVertex = static_cast<GLint>(m_dynamicOffset2D / sizeof(Vertex));
	glDrawArrays(ToOpenGLType(type), firstVertex, static_cast<GLsizei>(vertices.size()));

	m_dynamicOffset2D += bytesNeeded;
}

void OpenGLRenderAPI::SetDepthTest(const bool enabled)
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

void OpenGLRenderAPI::SetDepthMask(const bool writeEnabled)
{
	glDepthMask(writeEnabled ? GL_TRUE : GL_FALSE);
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

void OpenGLRenderAPI::DrawMesh3D(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform, const bool wireframe)
{
	if (vertices.empty() || indices.empty())
	{
		return;
	}

	m_Shader3D->Bind();
	m_Shader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_Shader3D->SetMat4("u_Model", transform);
	m_Shader3D->SetFloat4("u_Color", { 1.0f, 1.0f, 1.0f, 1.0f });

	const std::size_t vboBytesNeeded = vertices.size() * sizeof(Vertex);
	const std::size_t eboCountNeeded = indices.size();

	if (m_dynamicOffsetVbo3D + vboBytesNeeded > m_DynamicVBO3D->GetSize()
		|| m_dynamicOffsetEbo3D + eboCountNeeded > m_DynamicEBO3D->GetCapacity())
	{
		m_dynamicOffsetVbo3D = 0;
		m_dynamicOffsetEbo3D = 0;
	}

	m_DynamicVAO3D->Bind();

	m_DynamicVBO3D->SetData(vertices.data(), vboBytesNeeded, m_dynamicOffsetVbo3D);
	m_DynamicEBO3D->SetData(indices.data(), eboCountNeeded, m_dynamicOffsetEbo3D);

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1.0f, -1.0f);
	}

	const auto baseVertex = static_cast<GLint>(m_dynamicOffsetVbo3D / sizeof(Vertex));
	const auto indexByteOffset = reinterpret_cast<const void*>(m_dynamicOffsetEbo3D * sizeof(uint32_t));

	glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, indexByteOffset, baseVertex);

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}

	m_dynamicOffsetVbo3D += vboBytesNeeded;
	m_dynamicOffsetEbo3D += eboCountNeeded;
}

void OpenGLRenderAPI::DrawTexturedQuadImpl(const Vector3f& pos, const Vector2f& size, float rotation, Texture* texture, const Color& color)
{
	if (m_batchBuffer.size() >= 1000 * 4)
	{
		Flush();
	}

	float texIndex = -1.0f;
	if (texture)
	{
		Flush(); // TODO: Add Texture batching
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, *static_cast<uint32_t*>(texture->GetNativeHandle()));
		texIndex = 0.0f;
	}

	float hw = size.x / 2.0f;
	float hh = size.y / 2.0f;
	glm::vec2 p[4] = { { -hw, -hh }, { hw, -hh }, { hw, hh }, { -hw, hh } };

	if (rotation != 0.0f)
	{
		const float rad = glm::radians(rotation);
		const float s = std::sin(rad), c_rot = std::cos(rad);
		for (auto& i : p)
		{
			float x = i.x * c_rot - i.y * s;
			float y = i.x * s + i.y * c_rot;
			i = { x, y };
		}
	}

	constexpr Vector3f normal = { 0.f, 0.f, 1.f };
	m_batchBuffer.push_back({ { pos.x + p[0].x, pos.y + p[0].y, pos.z }, normal, color, { 0.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[1].x, pos.y + p[1].y, pos.z }, normal, color, { 1.0f, 0.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[2].x, pos.y + p[2].y, pos.z }, normal, color, { 1.0f, 1.0f }, texIndex });
	m_batchBuffer.push_back({ { pos.x + p[3].x, pos.y + p[3].y, pos.z }, normal, color, { 0.0f, 1.0f }, texIndex });
}

} // namespace re::render
