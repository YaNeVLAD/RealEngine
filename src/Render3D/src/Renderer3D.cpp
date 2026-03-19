#include <Render3D/Renderer3D.hpp>

#include <glm/ext/matrix_transform.hpp>

namespace re::render
{

std::unique_ptr<IRenderAPI> Renderer3D::m_api = nullptr;

void Renderer3D::Init(std::unique_ptr<IRenderAPI> api)
{
	m_api = std::move(api);
	m_api->Init();
}

void Renderer3D::BeginScene(
	const float fov,
	const float aspect,
	const float nearClip,
	const float farClip,
	const glm::mat4& viewMatrix)
{
	m_api->SetDepthTest(true);
	m_api->SetDepthMask(true);
	m_api->SetCameraPerspective(fov, aspect, nearClip, farClip, viewMatrix);
}

void Renderer3D::EndScene()
{
	m_api->SetDepthTest(false);
	m_api->SetDepthMask(true);
	m_api->SetCullMode(CullMode::None);
}

void Renderer3D::DrawMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const Vector3f& pos, const Vector3f& scale, const Vector3f& rotation, const bool wireframe)
{
	auto transform = glm::mat4(1.0f);
	transform = glm::translate(transform, { pos.x, pos.y, pos.z });

	// Rotation (Y, X, Z)
	transform = glm::rotate(transform, glm::radians(rotation.y), { 0.f, 1.f, 0.f });
	transform = glm::rotate(transform, glm::radians(rotation.x), { 1.f, 0.f, 0.f });
	transform = glm::rotate(transform, glm::radians(rotation.z), { 0.f, 0.f, 1.f });

	transform = glm::scale(transform, { scale.x, scale.y, scale.z });

	m_api->DrawMesh3D(vertices, indices, transform, wireframe);
}

void Renderer3D::DrawMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const glm::mat4& transform, bool wireframe)
{
	m_api->DrawMesh3D(vertices, indices, transform, wireframe);
}

void Renderer3D::DrawStaticMesh(StaticMesh* mesh, const glm::mat4& transform, bool wireframe)
{
	m_api->DrawStaticMesh3D(mesh, transform, wireframe);
}

void Renderer3D::DrawStaticMeshInstanced(StaticMesh* mesh, const std::vector<glm::mat4>& transforms, bool wireframe)
{
	m_api->DrawStaticMeshInstanced(mesh, transforms, wireframe);
}

void Renderer3D::DrawStaticMeshGPUCulled(const std::uint32_t batchIndex, StaticMesh* mesh, const std::vector<glm::mat4>& transforms, const float boundingRadius, const glm::vec3& cameraPos, const bool wireframe)
{
	m_api->DrawStaticMeshGPUCulled(batchIndex, mesh, transforms, boundingRadius, cameraPos, wireframe);
}

void Renderer3D::SetDirectionalLight(const glm::vec3& direction, const Color& color, float ambientIntensity)
{
	m_api->SetDirectionalLight(direction, color, ambientIntensity);
}

void Renderer3D::ResetCullingCache()
{
	m_api->ResetCullingCache();
}

void Renderer3D::SetViewport(const Vector2u size)
{
	m_api->SetViewport({ 0, 0 },
		{ static_cast<float>(size.x), static_cast<float>(size.y) });
}

void Renderer3D::SetDepthMask(const bool enable)
{
	m_api->SetDepthMask(enable);
}

void Renderer3D::SetCullMode(const CullMode mode)
{
	m_api->SetCullMode(mode);
}

} // namespace re::render