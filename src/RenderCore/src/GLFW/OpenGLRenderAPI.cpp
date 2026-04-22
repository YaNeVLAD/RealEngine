#include
#include
#include
#include

#include <RenderCore/GLFW/OpenGLRenderAPI.hpp>

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace
{

constexpr auto s_VertexEquirectToCube = UR"(
    #version 450 core
    layout (location = 0) in vec3 a_Position;

    out vec3 v_WorldPos;

    uniform mat4 u_ViewProjection;

    void main()
    {
        v_WorldPos = a_Position;
        gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_FragmentEquirectToCube = UR"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    in vec3 v_WorldPos;

    uniform sampler2D u_EquirectangularMap;

    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 SampleSphericalMap(vec3 v)
    {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= invAtan;
        uv += 0.5;
        return uv;
    }

    void main()
    {
        vec2 uv = SampleSphericalMap(normalize(v_WorldPos));
        vec3 color = texture(u_EquirectangularMap, uv).rgb;
        o_Color = vec4(color, 1.0);
    }
)";

constexpr auto s_FragmentIrradianceConvolution = UR"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    in vec3 v_WorldPos;

    uniform samplerCube u_EnvironmentMap;

    const float PI = 3.14159265359;

    void main()
    {
        vec3 N = normalize(v_WorldPos);
        vec3 irradiance = vec3(0.0);

        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, N));
        up = normalize(cross(N, right));

        float sampleDelta = 0.025;
        float nrSamples = 0.0;

        for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
        {
            for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
            {
                vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
                vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

                irradiance += texture(u_EnvironmentMap, sampleVec).rgb * cos(theta) * sin(theta);
                nrSamples++;
            }
        }
        irradiance = PI * irradiance * (1.0 / float(nrSamples));

        o_Color = vec4(irradiance, 1.0);
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
	[[maybe_unused]] std::uint32_t count;
	[[maybe_unused]] std::uint32_t instanceCount;
	[[maybe_unused]] std::uint32_t firstIndex;
	[[maybe_unused]] std::uint32_t baseVertex;
	[[maybe_unused]] std::uint32_t baseInstance;
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
		const float length = glm::length(glm::vec3(planes[i]));
		planes[i] /= length;
	}
}

bool HasNonUniformScale(const std::vector<glm::mat4>& transforms)
{
	return std::ranges::any_of(transforms, [](const auto& t) {
		const float l0 = glm::length(glm::vec3(t[0]));
		const float l1 = glm::length(glm::vec3(t[1]));
		const float l2 = glm::length(glm::vec3(t[2]));

		return std::abs(l0 - l1) > std::numeric_limits<float>::epsilon()
			|| std::abs(l0 - l2) > std::numeric_limits<float>::epsilon();
	});
}

template <typename Func>
void DrawWithWireframe(const bool wireframe, Func&& drawCall)
{
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1.0f, -1.0f);
		glLineWidth(5.0f);
	}

	drawCall();

	if (wireframe)
	{
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
}

void SetupMatrixAttribute(const GLuint location)
{
	constexpr std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(location + i);
		glVertexAttribPointer(location + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
		glVertexAttribDivisor(location + i, 1);
	}
}

void TeardownMatrixAttribute(const GLuint location)
{
	for (int i = 0; i < 4; i++)
	{
		glVertexAttribDivisor(location + i, 0);
		glDisableVertexAttribArray(location + i);
	}
}

glm::vec3 ColorToVec3(const re::Color c)
{
	return {
		static_cast<float>(c.r) / 255.f,
		static_cast<float>(c.g) / 255.f,
		static_cast<float>(c.b) / 255.f,
	};
}

class ScopedShaderRestorer
{
public:
	ScopedShaderRestorer()
	{
		glGetIntegerv(GL_CURRENT_PROGRAM, &m_previousProgram);
	}

	~ScopedShaderRestorer()
	{
		glUseProgram(m_previousProgram);
	}

	ScopedShaderRestorer(const ScopedShaderRestorer&) = delete;
	ScopedShaderRestorer& operator=(const ScopedShaderRestorer&) = delete;

private:
	GLint m_previousProgram = 0;
};

} // namespace

namespace re::render
{

void OpenGLRenderAPI::Init()
{
	using namespace file_system::literals;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	constexpr auto MaxQuads = 1000;
	constexpr auto MaxVertices = MaxQuads * 4;
	constexpr auto MaxIndices = MaxQuads * 6;

	m_batchBuffer.reserve(MaxVertices);

	m_Shader2D = std::make_shared<Shader>("dynamic2d.vert.glsl"_shader, "dynamic2d.frag.glsl");

	m_Shader3D = std::make_shared<Shader>("dynamic3d.vert.glsl"_shader, "phong.frag.glsl"_shader);
	m_InstancedShader3D = std::make_shared<Shader>("instantiated3d.vert.glsl"_shader, "phong.frag.glsl"_shader);

	m_Shader3DPBR = std::make_shared<Shader>("dynamic3d.vert.glsl"_shader, "pbr.frag.glsl"_shader);
	m_InstancedShader3DPBR = std::make_shared<Shader>("instantiated3d.vert.glsl"_shader, "pbr.frag.glsl"_shader);

	m_SkyboxShader3D = std::make_shared<Shader>("skybox.vert.glsl"_shader, "skybox.frag.glsl"_shader);

	glGenBuffers(1, &m_instanceVBOId);
	glGenBuffers(1, &m_normalVBOId);
	m_CullingComputeShader = std::make_shared<Shader>("compute/culling3d.comp.glsl"_shader);

	BufferLayout unifiedLayout;
	unifiedLayout.Push<Vector3f>("a_Position");
	unifiedLayout.Push<Vector3f>("a_Normal");
	unifiedLayout.Push<Color>("a_Color", true);
	unifiedLayout.Push<Vector2f>("a_TexCoord");
	unifiedLayout.Push<float>("a_TexIndex");
	unifiedLayout.Push<Vector4i>("a_BoneIDs");
	unifiedLayout.Push<Vector4f>("a_BoneWeights");
	unifiedLayout.Push<Vector4f>("a_Tangent");

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

	m_DynamicVAO = std::make_shared<VertexArray>();
	m_DynamicVBO = std::make_shared<VertexBuffer>(4096 * sizeof(Vertex));
	m_DynamicVAO->AddVertexBuffer(m_DynamicVBO, unifiedLayout);

	m_DynamicVAO3D = std::make_shared<VertexArray>();
	m_DynamicVBO3D = std::make_shared<VertexBuffer>(10000 * sizeof(Vertex));
	m_DynamicVAO3D->AddVertexBuffer(m_DynamicVBO3D, unifiedLayout);

	m_DynamicEBO3D = std::make_shared<IndexBuffer>(30000);
	m_DynamicVAO3D->SetIndexBuffer(m_DynamicEBO3D);

	constexpr float skyboxVertices[] = { // clang-format off
		-1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
	}; // clang-format on

	BufferLayout skyboxLayout;
	skyboxLayout.Push<Vector3f>("a_Position");

	m_CubeVAO3D = std::make_shared<VertexArray>();
	m_CubeVBO3D = std::make_shared<VertexBuffer>(sizeof(skyboxVertices));
	m_CubeVBO3D->SetData(skyboxVertices, sizeof(skyboxVertices));

	m_CubeVAO3D->AddVertexBuffer(m_CubeVBO3D, skyboxLayout);

	m_EquirectToCubeShader = std::make_shared<Shader>(s_VertexEquirectToCube, s_FragmentEquirectToCube);
	m_IrradianceShader = std::make_shared<Shader>(s_VertexEquirectToCube, s_FragmentIrradianceConvolution);
}

void OpenGLRenderAPI::ReloadShaders()
{
	m_Shader2D->Reload();
	m_Shader3D->Reload();
	m_InstancedShader3D->Reload();
	m_Shader3DPBR->Reload();
	m_InstancedShader3DPBR->Reload();
	m_SkyboxShader3D->Reload();
	m_CullingComputeShader->Reload();
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
		return;

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
{ // TODO: Implement circle drawing
	std::ignore = center, std::ignore = radius, std::ignore = color;
}

void OpenGLRenderAPI::DrawText(const String& text, const Font& font, const Vector3f& pos, const float fontSize, const Color& color)
{ // TODO: Implement text drawing
	std::ignore = text, std::ignore = font, std::ignore = pos, std::ignore = fontSize, std::ignore = color;
}

void OpenGLRenderAPI::DrawTexturedQuad(const Vector3f& pos, const Vector2f& size, Texture* texture, const Color& tint)
{
	DrawTexturedQuadImpl(pos, size, 0.0f, texture, tint);
}

void OpenGLRenderAPI::DrawMesh(std::vector<Vertex> const& vertices, const PrimitiveType type)
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
		glPointSize(5.f);
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
	for (auto& [inputSsbo, outputSsbo, cmdBuffer, normalSsbo] : m_cullingBatches)
	{
		glDeleteBuffers(1, &inputSsbo);
		glDeleteBuffers(1, &outputSsbo);
		glDeleteBuffers(1, &cmdBuffer);
		glDeleteBuffers(1, &normalSsbo);
	}
	m_cullingBatches.clear();
}

void OpenGLRenderAPI::SetCameraPerspective(const float fov, const float aspectRatio, const float nearClip, const float farClip, const glm::mat4& viewMatrix)
{
	const glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
	m_viewProj3D = projection * viewMatrix;
	const auto cameraPos = glm::vec3(glm::inverse(viewMatrix)[3]);

	m_Shader3D->Bind();
	m_Shader3D->SetFloat3("u_CameraPos", cameraPos);

	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_InstancedShader3D->SetFloat3("u_CameraPos", cameraPos);
}

void OpenGLRenderAPI::DrawMesh3D(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform, const bool wireframe)
{
	if (vertices.empty() || indices.empty())
	{
		return;
	}

	BindStandard3DUniforms(transform);

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

	DrawWithWireframe(wireframe, [&] {
		const auto baseVertex = static_cast<GLint>(m_dynamicOffsetVbo3D / sizeof(Vertex));
		const auto indexByteOffset = reinterpret_cast<const void*>(m_dynamicOffsetEbo3D * sizeof(std::uint32_t));

		glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, indexByteOffset, baseVertex);
	});

	m_dynamicOffsetVbo3D += vboBytesNeeded;
	m_dynamicOffsetEbo3D += eboCountNeeded;
}

void OpenGLRenderAPI::DrawStaticMesh3D(StaticMesh* mesh, const glm::mat4& transform, bool wireframe)
{
	if (!mesh)
	{
		return;
	}

	BindStandard3DUniforms(transform);

	mesh->GetVAO()->Bind();

	DrawWithWireframe(wireframe, [&] {
		glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	});
}

void OpenGLRenderAPI::DrawStaticMeshInstanced(StaticMesh* mesh, const std::vector<glm::mat4>& transforms, bool wireframe)
{
	if (transforms.empty() || !mesh)
	{
		return;
	}
	const bool nonUniform = HasNonUniformScale(transforms);

	{
		ScopedShaderRestorer restorer;

		m_InstancedShader3D->Bind();
		m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
		m_InstancedShader3D->SetBool("u_HasNonUniformScale", nonUniform);

		m_InstancedShader3DPBR->Bind();
		m_InstancedShader3DPBR->SetMat4("u_ViewProjection", m_viewProj3D);
		m_InstancedShader3DPBR->SetBool("u_HasNonUniformScale", nonUniform);
	}

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOId);
	glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_DYNAMIC_DRAW);
	SetupMatrixAttribute(8);

	std::vector<glm::mat4> normalMatrices;
	if (nonUniform)
	{
		normalMatrices.reserve(transforms.size());
		for (const auto& t : transforms)
		{
			normalMatrices.emplace_back(glm::transpose(glm::inverse(glm::mat3(t))));
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_normalVBOId);
		glBufferData(GL_ARRAY_BUFFER, normalMatrices.size() * sizeof(glm::mat4), normalMatrices.data(), GL_DYNAMIC_DRAW);
		SetupMatrixAttribute(12);
	}

	DrawWithWireframe(wireframe, [&] {
		glDrawElementsInstanced(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(transforms.size()));
	});

	TeardownMatrixAttribute(8);
	if (nonUniform)
	{
		TeardownMatrixAttribute(12);
	}
}

void OpenGLRenderAPI::DrawStaticMeshGPUCulled(const std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, const float boundingRadius, const glm::vec3& cameraPos, const bool wireframe, float farClip)
{
	if (transforms.empty() || !mesh)
	{
		return;
	}

	const auto totalInstances = static_cast<std::uint32_t>(transforms.size());

	if (batchIndex >= m_cullingBatches.size())
	{
		m_cullingBatches.resize(batchIndex + 1);
	}
	const bool nonUniform = HasNonUniformScale(transforms);
	auto& [inputSsbo, outputSsbo, cmdBuffer, normalSsbo] = m_cullingBatches[batchIndex];

	if (inputSsbo == 0)
	{
		glGenBuffers(1, &inputSsbo);
		glGenBuffers(1, &outputSsbo);
		glGenBuffers(1, &normalSsbo);
		glGenBuffers(1, &cmdBuffer);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(totalInstances * sizeof(glm::mat4)), transforms.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(totalInstances * sizeof(glm::mat4)), nullptr, GL_DYNAMIC_DRAW);

	if (nonUniform)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, normalSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(totalInstances * sizeof(glm::mat4)), nullptr, GL_DYNAMIC_DRAW);
	}

	const DrawElementsIndirectCommand cmd{ mesh->GetIndexCount(), 0, 0, 0, 0 };

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cmdBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsIndirectCommand), &cmd, GL_DYNAMIC_DRAW);

	glm::vec4 planes[6];
	ExtractFrustumPlanes(m_viewProj3D, planes);

	{ // SHADER_RESTORER_SCOPE_BEGIN
		ScopedShaderRestorer restorer;

		m_CullingComputeShader->Bind();
		m_CullingComputeShader->SetBool("u_ComputeNormals", nonUniform);
		m_CullingComputeShader->SetFloat4Array("u_FrustumPlanes", planes, 6);
		m_CullingComputeShader->SetUInt("u_TotalInstances", totalInstances);
		m_CullingComputeShader->SetFloat("u_MeshRadius", boundingRadius);
		m_CullingComputeShader->SetFloat3("u_CameraPos", cameraPos);
		m_CullingComputeShader->SetFloat("u_MaxDistance", farClip);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSsbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cmdBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, normalSsbo);

		const GLuint numGroups = (totalInstances + 63) / 64;
		glDispatchCompute(numGroups, 1, 1);

		glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		m_InstancedShader3D->Bind();
		m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
		m_InstancedShader3D->SetBool("u_HasNonUniformScale", nonUniform);

		m_InstancedShader3DPBR->Bind();
		m_InstancedShader3DPBR->SetMat4("u_ViewProjection", m_viewProj3D);
		m_InstancedShader3DPBR->SetBool("u_HasNonUniformScale", nonUniform);
	} // SHADER_RESTORER_SCOPE_END

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, outputSsbo);
	SetupMatrixAttribute(8);

	if (nonUniform)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normalSsbo);
		SetupMatrixAttribute(12);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBuffer);

	DrawWithWireframe(wireframe, [&] {
		// DEPTH PRE-PASS
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthFunc(GL_LESS);
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);

		// COLOR PASS
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);

		// CLEANUP
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	});

	TeardownMatrixAttribute(8);
	if (nonUniform)
	{
		TeardownMatrixAttribute(12);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderAPI::SetLight(const LightData& light)
{
	m_activeLight = light;
	const auto baseLightColor = light.diffuse;

	auto bindPhongLight = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();
		shader->SetInt("u_LightType", light.type);
		shader->SetFloat3("u_LightPos", light.position);
		shader->SetFloat3("u_LightDir", -light.direction);
		shader->SetFloat3("u_LightColor", baseLightColor);
		shader->SetFloat("u_LightConstant", light.constant);
		shader->SetFloat("u_LightLinear", light.linear);
		shader->SetFloat("u_LightQuadratic", light.quadratic);
		shader->SetFloat("u_LightCutOff", light.cutOff);
		shader->SetFloat("u_LightExponent", light.exponent);
	};

	auto bindPBRLight = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();
		shader->SetInt("u_LightType", light.type);
		shader->SetFloat3("u_LightPos", light.position);
		shader->SetFloat3("u_LightDir", -light.direction);

		constexpr float LIGHT_AMPLIFICATION = 2.5f;
		constexpr float PBR_INTENSITY = std::numbers::pi_v<float> * LIGHT_AMPLIFICATION;
		shader->SetFloat3("u_LightColor", baseLightColor * PBR_INTENSITY);
		shader->SetFloat3("u_LightAmbient", light.ambient);
		shader->SetFloat("u_LightConstant", light.constant);
		shader->SetFloat("u_LightLinear", light.linear);
		shader->SetFloat("u_LightQuadratic", light.quadratic);
		shader->SetFloat("u_LightCutOff", light.cutOff);
		shader->SetFloat("u_LightExponent", light.exponent);
	};

	bindPhongLight(m_Shader3D);
	bindPhongLight(m_InstancedShader3D);

	bindPBRLight(m_Shader3DPBR);
	bindPBRLight(m_InstancedShader3DPBR);
}

void OpenGLRenderAPI::SetMaterial(const Material& material)
{
	auto bindPhongMaterial = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();

		const glm::vec3 matAmb = ColorToVec3(material.albedoColor);
		const glm::vec3 matDif = ColorToVec3(material.albedoColor);
		const glm::vec3 matSpec = ColorToVec3(material.specularColor);
		const glm::vec3 matEmis = ColorToVec3(material.emissionColor);

		const glm::vec3 finalAmbient = m_activeLight.ambient * matAmb;
		const glm::vec3 finalDiffuse = m_activeLight.diffuse * matDif;
		const glm::vec3 finalSpecular = m_activeLight.specular * matSpec;

		shader->SetFloat3("u_MaterialAmbientLighting", finalAmbient);
		shader->SetFloat3("u_MaterialDiffuseLighting", finalDiffuse);
		shader->SetFloat3("u_MaterialSpecularLighting", finalSpecular);
		shader->SetFloat3("u_MaterialEmission", matEmis);
		shader->SetFloat("u_MaterialShininess", material.shininess);

		if (material.albedoMap)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.albedoMap->GetNativeHandle()));

			shader->SetInt("u_AlbedoMap", 0);
			shader->SetInt("u_HasAlbedoMap", 1);
		}
		else
		{
			shader->SetInt("u_HasAlbedoMap", 0);
		}

		if (material.normalMap)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.normalMap->GetNativeHandle()));

			shader->SetInt("u_NormalMap", 1);
			shader->SetInt("u_HasNormalMap", 1);
		}
		else
		{
			shader->SetInt("u_HasNormalMap", 0);
		}

		if (material.emissionMap)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.emissionMap->GetNativeHandle()));

			shader->SetInt("u_EmissionMap", 2);
			shader->SetInt("u_HasEmissionMap", 1);
		}
		else
		{
			shader->SetInt("u_HasEmissionMap", 0);
		}

		glActiveTexture(GL_TEXTURE0);
	};

	auto bindPBRMaterial = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();

		shader->SetFloat3("u_AlbedoColor", ColorToVec3(material.albedoColor));
		shader->SetFloat3("u_EmissionColor", ColorToVec3(material.emissionColor));
		shader->SetFloat("u_MetallicFactor", material.metallicFactor);
		shader->SetFloat("u_RoughnessFactor", material.roughnessFactor);

		if (material.albedoMap)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.albedoMap->GetNativeHandle()));

			shader->SetInt("u_AlbedoMap", 0);
			shader->SetInt("u_HasAlbedoMap", 1);
		}
		else
		{
			shader->SetInt("u_HasAlbedoMap", 0);
		}

		if (material.normalMap)
		{
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.normalMap->GetNativeHandle()));

			shader->SetInt("u_NormalMap", 1);
			shader->SetInt("u_HasNormalMap", 1);
		}
		else
		{
			shader->SetInt("u_HasNormalMap", 0);
		}

		if (material.emissionMap)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.emissionMap->GetNativeHandle()));

			shader->SetInt("u_EmissionMap", 2);
			shader->SetInt("u_HasEmissionMap", 1);
		}
		else
		{
			shader->SetInt("u_HasEmissionMap", 0);
		}

		if (material.metallicRoughnessMap)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.metallicRoughnessMap->GetNativeHandle()));

			shader->SetInt("u_MetallicRoughnessMap", 3);
			shader->SetInt("u_HasMetallicRoughnessMap", 1);
		}
		else
		{
			shader->SetInt("u_HasMetallicRoughnessMap", 0);
		}

		if (material.ambientOcclusionMap)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.ambientOcclusionMap->GetNativeHandle()));

			shader->SetInt("u_AOMap", 4);
			shader->SetInt("u_HasAOMap", 1);
		}
		else
		{
			shader->SetInt("u_HasAOMap", 0);
		}

		glActiveTexture(GL_TEXTURE0);
	};

	if (material.workflow == MaterialWorkflow::PBR)
	{
		bindPBRMaterial(m_Shader3DPBR);
		bindPBRMaterial(m_InstancedShader3DPBR);
	}
	else
	{
		bindPhongMaterial(m_Shader3D);
		bindPhongMaterial(m_InstancedShader3D);
	}
}

void OpenGLRenderAPI::DrawSkybox(const std::uint32_t cubemapID, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
	glDepthFunc(GL_LEQUAL);

	m_SkyboxShader3D->Bind();

	const auto view = glm::mat4(glm::mat3(viewMatrix));
	m_SkyboxShader3D->SetMat4("u_ViewProjection", projectionMatrix * view);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
	m_SkyboxShader3D->SetInt("u_EnvironmentMap", 0);

	m_CubeVAO3D->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
}

std::uint32_t OpenGLRenderAPI::CreateCubemapFromHDR(Texture* hdrTexture)
{
	if (!hdrTexture)
	{
		return 0;
	}

	constexpr int TEXTURE_SIZE = 1024;

	std::uint32_t captureFBO, captureRBO;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, TEXTURE_SIZE, TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	std::uint32_t envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
	};

	m_EquirectToCubeShader->Bind();
	m_EquirectToCubeShader->SetInt("u_EquirectangularMap", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(hdrTexture->GetNativeHandle()));

	glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	for (unsigned int i = 0; i < 6; ++i)
	{
		m_EquirectToCubeShader->SetMat4("u_ViewProjection", captureProjection * captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_CubeVAO3D->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glDeleteFramebuffers(1, &captureFBO);
	glDeleteRenderbuffers(1, &captureRBO);

	glViewport(
		static_cast<GLint>(m_viewport.x),
		static_cast<GLint>(m_viewport.y),
		static_cast<GLsizei>(m_viewport.z),
		static_cast<GLsizei>(m_viewport.w));

	return envCubemap;
}

std::uint32_t OpenGLRenderAPI::CreateIrradianceMap(const std::uint32_t envCubemap)
{
	if (envCubemap == 0)
	{
		return 0;
	}

	constexpr int TEXTURE_SIZE = 32;

	std::uint32_t captureFBO, captureRBO;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, TEXTURE_SIZE, TEXTURE_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	std::uint32_t irradianceMap;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
	};

	m_IrradianceShader->Bind();
	m_IrradianceShader->SetInt("u_EnvironmentMap", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	for (unsigned int i = 0; i < 6; ++i)
	{
		m_IrradianceShader->SetMat4("u_ViewProjection", captureProjection * captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_CubeVAO3D->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &captureFBO);
	glDeleteRenderbuffers(1, &captureRBO);

	glViewport(
		static_cast<GLint>(m_viewport.x),
		static_cast<GLint>(m_viewport.y),
		static_cast<GLsizei>(m_viewport.z),
		static_cast<GLsizei>(m_viewport.w));

	return irradianceMap;
}

void OpenGLRenderAPI::SetEnvironment(const std::uint32_t irradianceMap)
{
	auto bindEnv = [irradianceMap](const std::shared_ptr<Shader>& shader) {
		if (irradianceMap != 0)
		{
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
			shader->SetInt("u_IrradianceMap", 5);
			shader->SetInt("u_HasIrradianceMap", 1);
		}
		else
		{
			shader->SetInt("u_HasIrradianceMap", 0);
		}
	};

	bindEnv(m_Shader3DPBR);
	bindEnv(m_InstancedShader3DPBR);
}

void OpenGLRenderAPI::BindStandard3DUniforms(const glm::mat4& transform) const
{
	ScopedShaderRestorer shaderRestorer;

	const glm::mat4 mvp = m_viewProj3D * transform;
	const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

	m_Shader3D->Bind();
	m_Shader3D->SetMat4("u_ModelMatrix", transform);
	m_Shader3D->SetMat4("u_ModelViewProjection", mvp);
	m_Shader3D->SetMat4("u_NormalMatrix", normalMatrix);

	m_Shader3DPBR->Bind();
	m_Shader3DPBR->SetMat4("u_ModelMatrix", transform);
	m_Shader3DPBR->SetMat4("u_ModelViewProjection", mvp);
	m_Shader3DPBR->SetMat4("u_NormalMatrix", normalMatrix);
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

	float hw = size.x / 2.0f, hh = size.y / 2.0f;
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