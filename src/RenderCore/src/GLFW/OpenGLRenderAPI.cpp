#include "glm/gtc/type_ptr.hpp"

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

	uniform vec3 u_LightDir;
	uniform float u_Ambient;
	uniform vec4 u_LightColor;

    void main() {
		vec3 norm = normalize(v_Normal);
        vec3 lightDir = normalize(-u_LightDir);

        float diff = max(dot(norm, lightDir), 0.0);

        vec4 objectColor = v_Color * u_Color;

        vec3 finalLight = (u_Ambient + diff) * u_LightColor.rgb;

        o_Color = vec4(finalLight * objectColor.rgb, objectColor.a);
    }
)";

constexpr auto s_VertexInstancedShader3D = UR"(
    #version 450 core
    layout(location = 0) in vec3 a_Position;
    layout(location = 1) in vec3 a_Normal;
    layout(location = 2) in vec4 a_Color;
    layout(location = 3) in vec2 a_TexCoord;
    layout(location = 4) in float a_TexIndex;
	layout(location = 5) in mat4 a_InstanceMatrix;

    uniform mat4 u_ViewProjection;
    uniform mat4 u_Model;

    out vec4 v_Color;
    out vec2 v_TexCoord;
    out vec3 v_Normal;

    void main() {
        v_Color = a_Color;
        v_TexCoord = a_TexCoord;
		v_Normal = mat3(transpose(inverse(a_InstanceMatrix))) * a_Normal;
		gl_Position = u_ViewProjection * a_InstanceMatrix * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_ComputeCullingShader3D = UR"(
    #version 430 core
    layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

    layout(std430, binding = 0) readonly buffer InputInstances { mat4 inputTransforms[]; };
    layout(std430, binding = 1) writeonly buffer OutputInstances { mat4 outputTransforms[]; };

    struct DrawCommand 
	{
        uint count; 
		uint instanceCount; 
		uint firstIndex; 
		uint baseVertex; 
		uint baseInstance;
    };
    layout(std430, binding = 2) buffer CommandBuffer { DrawCommand cmd; };

    uniform vec4 u_FrustumPlanes[6];
    uniform uint u_TotalInstances;
    uniform float u_MeshRadius;

    uniform vec3 u_CameraPos;
    uniform float u_MaxDistance;

    void main()
    {
        uint idx = gl_GlobalInvocationID.x;
        if (idx >= u_TotalInstances) return;

        mat4 model = inputTransforms[idx];
        vec3 pos = vec3(model[3]);

        // DISTANCE CULLING
        // Если объект дальше условного горизонта, сразу выбрасываем его!
        if (distance(u_CameraPos, pos) > u_MaxDistance) return;

        float maxScale = max(max(length(vec3(model[0])), length(vec3(model[1]))), length(vec3(model[2])));
        float radius = u_MeshRadius * maxScale;

        // FRUSTUM CULLING
        bool visible = true;
        for(int i = 0; i < 6; i++)
        {
            if (dot(u_FrustumPlanes[i].xyz, pos) + u_FrustumPlanes[i].w < -radius)
            {
                visible = false;
                break;
            }
        }

        if (visible)
        {
            uint outIdx = atomicAdd(cmd.instanceCount, 1);
            outputTransforms[outIdx] = model;
        }
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

struct DrawElementsIndirectCommand
{
	uint32_t count; // mesh->GetIndexCount()
	uint32_t instanceCount; // 0 (заполнит Compute Shader)
	uint32_t firstIndex; // 0
	uint32_t baseVertex; // 0
	uint32_t baseInstance; // 0
};

void ExtractFrustumPlanes(const glm::mat4& vp, glm::vec4 planes[6])
{
	// clang-format off

	// Left, Right, Bottom, Top, Near, Far
	for (int i = 0; i < 4; ++i) planes[0][i] = vp[i][3] + vp[i][0];
	for (int i = 0; i < 4; ++i) planes[1][i] = vp[i][3] - vp[i][0];
	for (int i = 0; i < 4; ++i) planes[2][i] = vp[i][3] + vp[i][1];
	for (int i = 0; i < 4; ++i) planes[3][i] = vp[i][3] - vp[i][1];
	for (int i = 0; i < 4; ++i) planes[4][i] = vp[i][3] + vp[i][2];
	for (int i = 0; i < 4; ++i) planes[5][i] = vp[i][3] - vp[i][2];

	// clang-format on

	for (int i = 0; i < 6; i++)
	{
		float length = glm::length(glm::vec3(planes[i]));
		planes[i] /= length;
	}
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

	m_InstancedShader3D = std::make_shared<Shader>(s_VertexInstancedShader3D, s_FragmentShader3D);
	glGenBuffers(1, &m_instanceVBOId);

	m_CullingComputeShader = std::make_shared<Shader>(s_ComputeCullingShader3D);

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

void OpenGLRenderAPI::SetCullMode(const CullMode mode)
{
	if (mode == CullMode::None)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
		glCullFace(mode == CullMode::Front ? GL_FRONT : GL_BACK);
	}
}

void OpenGLRenderAPI::ResetCullingCache()
{
	for (auto& [inputSsbo, outputSsbo, cmdBuffer] : m_cullingBatches)
	{
		glDeleteBuffers(1, &inputSsbo);
		glDeleteBuffers(1, &outputSsbo);
		glDeleteBuffers(1, &cmdBuffer);
	}
	m_cullingBatches.clear();
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

	m_Shader3D->Bind();
	m_Shader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_Shader3D->SetFloat4("u_Color", { 1.0f, 1.0f, 1.0f, 1.0f });
}

void OpenGLRenderAPI::DrawMesh3D(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform, const bool wireframe)
{
	if (vertices.empty() || indices.empty())
	{
		return;
	}

	m_Shader3D->Bind();
	m_Shader3D->SetMat4("u_Model", transform);

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
		glLineWidth(5.0f);
	}

	const auto baseVertex = static_cast<GLint>(m_dynamicOffsetVbo3D / sizeof(Vertex));
	const auto indexByteOffset = reinterpret_cast<const void*>(m_dynamicOffsetEbo3D * sizeof(uint32_t));

	glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, indexByteOffset, baseVertex);

	if (wireframe)
	{
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}

	m_dynamicOffsetVbo3D += vboBytesNeeded;
	m_dynamicOffsetEbo3D += eboCountNeeded;
}

void OpenGLRenderAPI::DrawStaticMesh3D(StaticMesh* mesh, const glm::mat4& transform, bool wireframe)
{
	if (!mesh)
	{
		return;
	}

	m_Shader3D->Bind();
	m_Shader3D->SetMat4("u_Model", transform);

	mesh->GetVAO()->Bind();

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1.0f, -1.0f);
		glLineWidth(5.0f);
	}

	glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);

	if (wireframe)
	{
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
}

void OpenGLRenderAPI::DrawStaticMeshInstanced(StaticMesh* mesh, const std::vector<glm::mat4>& transforms, bool wireframe)
{
	if (transforms.empty() || !mesh)
	{
		return;
	}

	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_InstancedShader3D->SetFloat4("u_Color", { 1.0f, 1.0f, 1.0f, 1.0f });

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOId);
	glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_DYNAMIC_DRAW);

	constexpr std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(5 + i);
		glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));

		// МАГИЯ ЗДЕСЬ: Divisor=1 означает "обновлять этот атрибут только при переходе к следующему инстансу (мешу)"
		glVertexAttribDivisor(5 + i, 1);
	}

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1.0f, -1.0f);
		glLineWidth(5.0f);
	}

	glDrawElementsInstanced(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(transforms.size()));

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glLineWidth(1.0f);
	}

	for (int i = 0; i < 4; i++)
	{
		glVertexAttribDivisor(5 + i, 0);
		glDisableVertexAttribArray(5 + i);
	}
}

void OpenGLRenderAPI::DrawStaticMeshGPUCulled(const std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, const float boundingRadius, const glm::vec3& cameraPos, const bool wireframe)
{
	if (transforms.empty() || !mesh)
	{
		return;
	}

	const std::uint32_t totalInstances = static_cast<uint32_t>(transforms.size());

	// Увеличиваем массив буферов, если пришел новый батч
	if (batchIndex >= m_cullingBatches.size())
	{
		m_cullingBatches.resize(batchIndex + 1);
	}
	auto& batch = m_cullingBatches[batchIndex];

	// 1. ИНИЦИАЛИЗАЦИЯ И ЗАГРУЗКА ДАННЫХ (ТОЛЬКО ОДИН РАЗ!)
	if (batch.inputSsbo == 0)
	{
		glGenBuffers(1, &batch.inputSsbo);
		glGenBuffers(1, &batch.outputSsbo);
		glGenBuffers(1, &batch.cmdBuffer);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.inputSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, totalInstances * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.outputSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, totalInstances * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
	}

	// 2. ОБНУЛЕНИЕ СЧЕТЧИКА (Каждый кадр)
	DrawElementsIndirectCommand cmd{};
	cmd.count = mesh->GetIndexCount();
	cmd.instanceCount = 0;
	cmd.firstIndex = 0;
	cmd.baseVertex = 0;
	cmd.baseInstance = 0;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, batch.cmdBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsIndirectCommand), &cmd, GL_DYNAMIC_DRAW);

	// 3. НАСТРОЙКА COMPUTE SHADER

	glm::vec4 planes[6];
	ExtractFrustumPlanes(m_viewProj3D, planes);

	m_CullingComputeShader->Bind();
	m_CullingComputeShader->SetFloat4Array("u_FrustumPlanes", planes, 6);
	m_CullingComputeShader->SetUInt("u_TotalInstances", totalInstances);
	m_CullingComputeShader->SetFloat("u_MeshRadius", boundingRadius);
	m_CullingComputeShader->SetFloat3("u_CameraPos", cameraPos);
	m_CullingComputeShader->SetFloat("u_MaxDistance", 150.0f);

	// ИСПОЛЬЗУЕМ БУФЕРЫ КОНКРЕТНОГО БАТЧА
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.inputSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batch.outputSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch.cmdBuffer);

	GLuint numGroups = (totalInstances + 63) / 64;
	glDispatchCompute(numGroups, 1, 1);

	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	// 4. ОТРИСОВКА (Early-Z и Color)
	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_InstancedShader3D->SetFloat4("u_Color", { 1.0f, 1.0f, 1.0f, 1.0f });

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, batch.outputSsbo);
	constexpr std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(5 + i);
		glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
		glVertexAttribDivisor(5 + i, 1);
	}

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1.0f, -1.0f);
		glLineWidth(5.0f);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.cmdBuffer);

	// --- ПРОХОД 1: DEPTH PRE-PASS ---
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);

	// --- ПРОХОД 2: COLOR PASS ---
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);

	// --- УБОРКА ---
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glLineWidth(1.0f);
	}

	for (int i = 0; i < 4; i++)
	{
		glVertexAttribDivisor(5 + i, 0);
		glDisableVertexAttribArray(5 + i);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderAPI::SetDirectionalLight(const glm::vec3& direction, const Color& color, float ambientIntensity)
{
	m_lightDir = direction;
	m_lightColor = { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f };
	m_lightAmbient = ambientIntensity;

	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetFloat3("u_LightDir", m_lightDir);
	m_InstancedShader3D->SetFloat4("u_LightColor", m_lightColor);
	m_InstancedShader3D->SetFloat("u_Ambient", m_lightAmbient);

	m_Shader3D->Bind();
	m_Shader3D->SetFloat3("u_LightDir", m_lightDir);
	m_Shader3D->SetFloat4("u_LightColor", m_lightColor);
	m_Shader3D->SetFloat("u_Ambient", m_lightAmbient);
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
		glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(texture->GetNativeHandle()));
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
