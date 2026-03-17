#include <Runtime/Internal/RenderSystem3D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render3D/Renderer3D.hpp>
#include <Runtime/Components.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <algorithm>

namespace re::detail
{

RenderSystem3D::RenderSystem3D(render::IWindow& window)
	: m_window(window)
{
}

void RenderSystem3D::Update(ecs::Scene& scene, float)
{
	m_opaqueQueue.clear();
	m_transparentQueue.clear();

	auto glmCamPos = glm::vec3{ 0.f, 0.f, 0.f };
	auto viewMatrix = glm::mat4(1.0f);

	float fov = 45.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;

	auto [width, height] = m_window.Size();

	const float aspect = (float)width / (float)height;
	for (auto&& [entity, transform, camera] : *scene.CreateView<TransformComponent, CameraComponent>())
	{
		if (camera.isPrimal)
		{
			glmCamPos = { transform.position.x, transform.position.y, transform.position.z };

			fov = camera.fov;
			nearClip = camera.nearClip;
			farClip = camera.farClip;

			// (transform.rotation.y = Yaw, transform.rotation.x = Pitch)
			glm::vec3 front;
			float yaw = transform.rotation.y;
			float pitch = transform.rotation.x;

			front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front.y = std::sin(glm::radians(pitch));
			front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front = glm::normalize(front);

			auto up = glm::vec3{ 0.f, 1.f, 0.f };
			viewMatrix = glm::lookAt(glmCamPos, glmCamPos + front, up);
			break;
		}
	}

	for (auto&& [entity, transform, mesh] : *scene.CreateView<TransformComponent, MeshComponent3D>())
	{
		if (mesh.vertices.empty() || mesh.indices.empty())
		{
			continue;
		}

		auto modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(transform.position.x, transform.position.y, transform.position.z));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(transform.scale.x, transform.scale.y, transform.scale.z));

		const bool isTransparent = mesh.vertices[0].color.a < 255;

		RenderCommand3D cmd{
			.transform = modelMatrix,
			.vertices = &mesh.vertices,
			.indices = &mesh.indices,
			.wireframe = mesh.wireframe,
		};
		if (isTransparent)
		{
			auto worldPos = glm::vec3(modelMatrix[3]);
			cmd.distanceToCamera = glm::distance(glmCamPos, worldPos);
			m_transparentQueue.push_back(cmd);
		}
		else
		{
			m_opaqueQueue.push_back(cmd);
		}
	}

	render::Renderer3D::BeginScene(fov, aspect, nearClip, farClip, viewMatrix);

	for (const auto& cmd : m_opaqueQueue)
	{
		render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
	}

	if (!m_transparentQueue.empty())
	{
		std::ranges::sort(m_transparentQueue, [](const RenderCommand3D& a, const RenderCommand3D& b) {
			return a.distanceToCamera > b.distanceToCamera;
		});

		render::Renderer3D::SetDepthMask(false);
		render::Renderer3D::SetCullMode(render::CullMode::Front);
		for (const auto& cmd : m_transparentQueue)
		{
			render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
		}

		render::Renderer3D::SetCullMode(render::CullMode::Back);
		for (const auto& cmd : m_transparentQueue)
		{
			render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
		}
	}

	render::Renderer3D::EndScene();
}

} // namespace re::detail