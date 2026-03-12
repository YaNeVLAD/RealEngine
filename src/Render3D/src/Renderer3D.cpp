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

void Renderer3D::DrawCube(
	Vector3f const& position,
	Vector3f const& rotation,
	Vector3f const& scale,
	Color const& color)
{
	auto transform = glm::mat4(1.0f);

	transform = glm::translate(transform, { position.x, position.y, position.z });

	transform = glm::rotate(transform, glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f });
	transform = glm::rotate(transform, glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f });
	transform = glm::rotate(transform, glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f });

	transform = glm::scale(transform, { scale.x, scale.y, scale.z });

	m_api->DrawCube(transform, color);
}

void Renderer3D::SetViewport(const Vector2u size)
{
	m_api->SetViewport({ 0, 0 }, { (float)size.x, (float)size.y });
}

} // namespace re::render