#include <Runtime/Internal/RenderSystem3D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render3D/Renderer3D.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <iostream>

namespace re::detail
{

RenderSystem3D::RenderSystem3D(render::IWindow& window)
	: m_window(window)
{
}

void RenderSystem3D::Update(ecs::Scene& scene, float)
{
	auto [width, height] = m_window.Size();
	const auto w = std::max(width, 1u);
	const auto h = std::max(height, 1u);
	const float aspectRatio = static_cast<float>(w) / static_cast<float>(h);

	const glm::mat4 viewMatrix = glm::lookAt(
		glm::vec3(0.f, 0.f, 0.f), // Position
		glm::vec3(0.f, 0.f, -1.f), // View to -Z
		glm::vec3(0.f, 1.f, 0.f) // Up vector
	);

	render::Renderer3D::BeginScene(45.0f, aspectRatio, viewMatrix);

	for (auto&& [entity, transform, cube] : *scene.CreateView<TransformComponent, CubeComponent>())
	{
		auto [cubeVertices, cubeIndices] = PrimitiveBuilder::CreateCube(cube.color);
		render::Renderer3D::DrawMesh(
			cubeVertices,
			cubeIndices,
			transform.position,
			transform.scale,
			transform.rotation);
	}

	for (const auto&& [_, transform, mesh] : *scene.CreateView<TransformComponent, MeshComponent3D>())
	{
		render::Renderer3D::DrawMesh(
			mesh.vertices,
			mesh.indices,
			transform.position,
			transform.scale,
			transform.rotation);
	}

	render::Renderer3D::EndScene();
}

} // namespace re::detail