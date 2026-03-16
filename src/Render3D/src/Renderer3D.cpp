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

void Renderer3D::BeginScene(const float fov, const float aspect, const glm::mat4& viewMatrix)
{
	m_api->SetDepthTest(true);
	m_api->SetCameraPerspective(fov, aspect, 0.1f, 1000.0f, viewMatrix);
}

void Renderer3D::EndScene()
{
	m_api->SetDepthTest(false);
}

void Renderer3D::DrawMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const Vector3f& pos, const Vector3f& scale, const Vector3f& rotation)
{
	auto transform = glm::mat4(1.0f);
	transform = glm::translate(transform, { pos.x, pos.y, pos.z });

	// Rotation (Y, X, Z)
	transform = glm::rotate(transform, glm::radians(rotation.y), { 0.f, 1.f, 0.f });
	transform = glm::rotate(transform, glm::radians(rotation.x), { 1.f, 0.f, 0.f });
	transform = glm::rotate(transform, glm::radians(rotation.z), { 0.f, 0.f, 1.f });

	transform = glm::scale(transform, { scale.x, scale.y, scale.z });

	m_api->DrawMesh3D(vertices, indices, transform);
}

void Renderer3D::SetViewport(const Vector2u size)
{
	m_api->SetViewport({ 0, 0 },
		{ static_cast<float>(size.x), static_cast<float>(size.y) });
}

} // namespace re::render