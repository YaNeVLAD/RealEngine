#include <RenderCore/GLFW/OpenGLRenderAPI.hpp>

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

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

    void main()
	{
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

    void main()
	{
        if (v_TexIndex < 0.0)
		{
            color = v_Color;
        }
		else
		{
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

	uniform mat4 u_ModelViewProjection;
	uniform mat4 u_ModelMatrix;
	uniform mat3 u_NormalMatrix;

    out vec4 v_Color;
    out vec2 v_TexCoord;
    out vec3 v_Normal;
	out vec3 v_FragmentWorldPos;

    void main() {
        v_Color = a_Color;
        v_TexCoord = a_TexCoord;
		v_Normal = u_NormalMatrix * a_Normal;
		v_FragmentWorldPos = vec3(u_ModelMatrix * vec4(a_Position, 1.0));
		gl_Position = u_ModelViewProjection * vec4(a_Position, 1.0);
    }
)";

constexpr auto s_FragmentShader3D = UR"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    in vec4 v_Color;
    in vec2 v_TexCoord;
    in vec3 v_Normal;
    in vec3 v_FragmentWorldPos;

    uniform vec3 u_MaterialAmbientLighting;
    uniform vec3 u_MaterialDiffuseLighting;
    uniform vec3 u_MaterialSpecularLighting;
    uniform vec3 u_MaterialEmission;
    uniform float u_MaterialShininess;

	uniform sampler2D u_Texture;
    uniform int u_HasTexture;

    uniform int u_LightType;
    uniform vec3 u_LightPos;
    uniform vec3 u_LightDir;
    uniform float u_LightConstant;
    uniform float u_LightLinear;
    uniform float u_LightQuadratic;
    uniform float u_LightCutOff;
    uniform float u_LightExponent;

    uniform vec3 u_CameraPos;

    void main()
	{
        vec3 surfaceNormal = normalize(v_Normal);
        vec3 viewDir = normalize(u_CameraPos - v_FragmentWorldPos);
        vec3 lightDir;

        float attenuation = 1.0;
        float spotEffect = 1.0;

        if (u_LightType == 0)
		{ // Directional
            lightDir = normalize(u_LightDir);
        }
		else
		{ // Point or Spotlight
            lightDir = normalize(u_LightPos - v_FragmentWorldPos);
            float dist = length(u_LightPos - v_FragmentWorldPos);
            attenuation = 1.0 / (u_LightConstant + u_LightLinear * dist + u_LightQuadratic * (dist * dist));

            if (u_LightType == 2)
			{ // Spotlight
                float cosAlpha = dot(lightDir, normalize(u_LightDir));
                if (cosAlpha > u_LightCutOff)
				{
					spotEffect = pow(cosAlpha, u_LightExponent);
                }
				else
				{
                    spotEffect = 0.0;
                }
            }
        }

		// TEXTURE
		vec4 surfaceColor = v_Color;
        if (u_HasTexture != 0) {
            surfaceColor *= texture(u_Texture, v_TexCoord);
        }

		// Ambient
        vec3 ambientLight = u_MaterialAmbientLighting;

        // Diffuse
		float diffuseIntensity = max(dot(surfaceNormal, lightDir), 0.0);
		vec3 diffuseLight = u_MaterialDiffuseLighting * diffuseIntensity;

        // Specular
		vec3 reflectDir = reflect(lightDir, surfaceNormal);
        float specFactor = pow(max(dot(reflectDir, viewDir), 0.0), u_MaterialShininess);
        vec3 specularLight = u_MaterialSpecularLighting * specFactor;

		vec3 finalAmbient = ambientLight * surfaceColor.rgb;
       	vec3 finalDiffuse = diffuseLight * surfaceColor.rgb * attenuation * spotEffect;
       	vec3 finalSpecular = specularLight * attenuation * spotEffect;

		vec3 finalColor = u_MaterialEmission + finalAmbient + finalDiffuse + finalSpecular;

		o_Color = vec4(finalColor, surfaceColor.a);
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
    layout(location = 9) in mat4 a_NormalMatrix;

    uniform mat4 u_ViewProjection;
    uniform bool u_HasNonUniformScale;

    out vec4 v_Color;
    out vec2 v_TexCoord;
    out vec3 v_Normal;
    out vec3 v_FragmentWorldPos;

    void main()
	{
        v_Color = a_Color;
        v_TexCoord = a_TexCoord;

        if (u_HasNonUniformScale)
		{
            v_Normal = mat3(a_NormalMatrix) * a_Normal;
        }
		else
		{
            v_Normal = mat3(a_InstanceMatrix) * a_Normal;
        }

        vec4 worldPos = a_InstanceMatrix * vec4(a_Position, 1.0);
        v_FragmentWorldPos = vec3(worldPos);
        gl_Position = u_ViewProjection * worldPos;
    }
)";

constexpr auto s_ComputeCullingShader3D = UR"(
    #version 430 core
    layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

    layout(std430, binding = 0) readonly buffer InputInstances { mat4 inputTransforms[]; };
    layout(std430, binding = 1) writeonly buffer OutputInstances { mat4 outputTransforms[]; };
	layout(std430, binding = 3) writeonly buffer OutputNormals { mat4 outputNormalMatrices[]; };

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

	uniform bool u_ComputeNormals;

    void main()
    {
        uint idx = gl_GlobalInvocationID.x;
        if (idx >= u_TotalInstances) return;

        mat4 model = inputTransforms[idx];
        vec3 pos = vec3(model[3]);

        // DISTANCE CULLING
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
			if (u_ComputeNormals)
			{
                outputNormalMatrices[outIdx] = mat4(transpose(inverse(mat3(model))));
            }
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

void BindStandard3DUniforms(const std::shared_ptr<re::render::Shader>& shader, const glm::mat4& transform, const glm::mat4& viewProj)
{
	shader->Bind();
	shader->SetMat4("u_ModelViewProjection", viewProj * transform);
	shader->SetMat4("u_ModelMatrix", transform);
	shader->SetMat4("u_NormalMatrix", glm::transpose(glm::inverse(glm::mat3(transform))));
}

glm::vec4 ColorToVec4(const re::Color c)
{
	return {
		static_cast<float>(c.r) / 255.f,
		static_cast<float>(c.g) / 255.f,
		static_cast<float>(c.b) / 255.f,
		static_cast<float>(c.a) / 255.f,
	};
}

glm::vec3 ColorToVec3(const re::Color c)
{
	return {
		static_cast<float>(c.r) / 255.f,
		static_cast<float>(c.g) / 255.f,
		static_cast<float>(c.b) / 255.f,
	};
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
	glGenBuffers(1, &m_normalVBOId);
	m_CullingComputeShader = std::make_shared<Shader>(s_ComputeCullingShader3D);

	BufferLayout unifiedLayout;
	unifiedLayout.Push<Vector3f>("a_Position");
	unifiedLayout.Push<Vector3f>("a_Normal");
	unifiedLayout.Push<Color>("a_Color", true);
	unifiedLayout.Push<Vector2f>("a_TexCoord");
	unifiedLayout.Push<float>("a_TexIndex");

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

	BindStandard3DUniforms(m_Shader3D, transform, m_viewProj3D);

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

	BindStandard3DUniforms(m_Shader3D, transform, m_viewProj3D);
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

	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);
	m_InstancedShader3D->SetBool("u_HasNonUniformScale", nonUniform);

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOId);
	glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_DYNAMIC_DRAW);
	SetupMatrixAttribute(5);

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
		SetupMatrixAttribute(9);
	}

	DrawWithWireframe(wireframe, [&] {
		glDrawElementsInstanced(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(transforms.size()));
	});

	TeardownMatrixAttribute(5);
	if (nonUniform)
	{
		TeardownMatrixAttribute(9);
	}
}

void OpenGLRenderAPI::DrawStaticMeshGPUCulled(const std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, const float boundingRadius, const glm::vec3& cameraPos, const bool wireframe)
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

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, totalInstances * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, totalInstances * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, normalSsbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, totalInstances * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
	}

	const DrawElementsIndirectCommand cmd{ mesh->GetIndexCount(), 0, 0, 0, 0 };

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cmdBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsIndirectCommand), &cmd, GL_DYNAMIC_DRAW);

	glm::vec4 planes[6];
	ExtractFrustumPlanes(m_viewProj3D, planes);

	m_CullingComputeShader->Bind();
	m_CullingComputeShader->SetBool("u_ComputeNormals", nonUniform);
	m_CullingComputeShader->SetFloat4Array("u_FrustumPlanes", planes, 6);
	m_CullingComputeShader->SetUInt("u_TotalInstances", totalInstances);
	m_CullingComputeShader->SetFloat("u_MeshRadius", boundingRadius);
	m_CullingComputeShader->SetFloat3("u_CameraPos", cameraPos);
	m_CullingComputeShader->SetFloat("u_MaxDistance", 1000.0f);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cmdBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, normalSsbo);

	const GLuint numGroups = (totalInstances + 63) / 64;
	glDispatchCompute(numGroups, 1, 1);

	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	m_InstancedShader3D->Bind();
	m_InstancedShader3D->SetBool("u_HasNonUniformScale", nonUniform);
	m_InstancedShader3D->SetMat4("u_ViewProjection", m_viewProj3D);

	mesh->GetVAO()->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, outputSsbo);
	SetupMatrixAttribute(5);

	if (nonUniform)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normalSsbo);
		SetupMatrixAttribute(9);
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

	TeardownMatrixAttribute(5);
	if (nonUniform)
	{
		TeardownMatrixAttribute(9);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLRenderAPI::SetLight(const LightData& light)
{
	m_activeLight = light;

	auto bindLightToShader = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();
		shader->SetInt("u_LightType", light.type);
		shader->SetFloat3("u_LightPos", light.position);
		shader->SetFloat3("u_LightDir", -light.direction);
		shader->SetFloat("u_LightConstant", light.constant);
		shader->SetFloat("u_LightLinear", light.linear);
		shader->SetFloat("u_LightQuadratic", light.quadratic);
		shader->SetFloat("u_LightCutOff", light.cutOff);
		shader->SetFloat("u_LightExponent", light.exponent);
	};

	bindLightToShader(m_Shader3D);
	bindLightToShader(m_InstancedShader3D);
}

void OpenGLRenderAPI::SetMaterial(const Material& material)
{
	const glm::vec3 matAmb = ColorToVec3(material.ambient);
	const glm::vec3 matDif = ColorToVec3(material.diffuse);
	const glm::vec3 matSpec = ColorToVec3(material.specular);
	const glm::vec3 matEmis = ColorToVec3(material.emission);

	const glm::vec3 finalAmbient = m_activeLight.ambient * matAmb;
	const glm::vec3 finalDiffuse = m_activeLight.diffuse * matDif;
	const glm::vec3 finalSpecular = m_activeLight.specular * matSpec;

	auto bindMaterialToShader = [&](const std::shared_ptr<Shader>& shader) {
		shader->Bind();
		shader->SetFloat3("u_MaterialAmbientLighting", finalAmbient);
		shader->SetFloat3("u_MaterialDiffuseLighting", finalDiffuse);
		shader->SetFloat3("u_MaterialSpecularLighting", finalSpecular);
		shader->SetFloat3("u_MaterialEmission", matEmis);
		shader->SetFloat("u_MaterialShininess", material.shininess);

		glActiveTexture(GL_TEXTURE0);
		if (material.texture)
		{
			shader->SetInt("u_Texture", 0);
			shader->SetInt("u_HasTexture", 1);

			glBindTexture(GL_TEXTURE_2D, *static_cast<std::uint32_t*>(material.texture->GetNativeHandle()));
		}
		else
		{
			shader->SetInt("u_HasTexture", 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};

	bindMaterialToShader(m_Shader3D);
	bindMaterialToShader(m_InstancedShader3D);
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