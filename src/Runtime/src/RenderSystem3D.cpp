#include <Runtime/Internal/RenderSystem3D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render3D/Renderer3D.hpp>
#include <Runtime/Components.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_INLINE
#define GLM_FORCE_INTRINSICS
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <algorithm>
#include <execution>

namespace
{

glm::mat4 CalculateModelMatrix(re::TransformComponent const& transform)
{
	glm::mat4 result = glm::translate(glm::mat4(1.0f), glm::vec3(transform.position.x, transform.position.y, transform.position.z));

	result *= glm::eulerAngleYXZ(
		glm::radians(transform.rotation.y),
		glm::radians(transform.rotation.x),
		glm::radians(transform.rotation.z));

	result = glm::scale(result, glm::vec3(transform.scale.x, transform.scale.y, transform.scale.z));

	return result;
}

} // namespace

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
	m_instancedOpaque.clear();
	m_instancedTransparent.clear();

	auto glmCamPos = glm::vec3{ 0.f, 0.f, 0.f };
	auto viewMatrix = glm::mat4(1.0f);

	float fov = 45.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;

	auto [width, height] = m_window.Size();
	const float aspect = (float)width / (float)height;

	for (auto&& [entity, transform] : *scene.CreateView<TransformComponent>())
	{
		if (transform.isDirty)
		{
			transform.modelMatrix = CalculateModelMatrix(transform);
			transform.isDirty = false;
		}
	}

	for (auto&& [entity, transform, camera] : *scene.CreateView<TransformComponent, CameraComponent>())
	{
		if (camera.isPrimal)
		{
			glmCamPos = { transform.position.x, transform.position.y, transform.position.z };

			fov = camera.fov;
			nearClip = camera.nearClip;
			farClip = camera.farClip;

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

	for (auto&& [entity, transform, staticComp] : *scene.CreateView<TransformComponent, StaticMeshComponent3D>())
	{
		if (!staticComp.mesh)
		{
			continue;
		}

		glm::mat4 modelMatrix = transform.modelMatrix;
		auto key = std::make_pair(staticComp.mesh.get(), staticComp.wireframe);

		if (staticComp.mesh->IsTransparent())
		{
			float dist = glm::distance(glmCamPos, glm::vec3(modelMatrix[3]));
			m_instancedTransparent[key].emplace_back(dist, modelMatrix);
		}
		else
		{
			m_instancedOpaque[key].push_back(modelMatrix);
		}
	}

	for (auto&& [entity, transform, dynComp] : *scene.CreateView<TransformComponent, DynamicMeshComponent3D>())
	{
		if (dynComp.vertices.empty() || dynComp.indices.empty())
		{
			continue;
		}

		glm::mat4 modelMatrix = transform.modelMatrix;

		RenderCommand3D cmd;
		cmd.transform = modelMatrix;
		cmd.vertices = &dynComp.vertices;
		cmd.indices = &dynComp.indices;
		cmd.wireframe = dynComp.wireframe;

		if (dynComp.vertices[0].color.a < 255)
		{
			cmd.distanceToCamera = glm::distance(glmCamPos, glm::vec3(modelMatrix[3]));
			m_transparentQueue.push_back(cmd);
		}
		else
		{
			m_opaqueQueue.push_back(cmd);
		}
	}

	render::Renderer3D::BeginScene(fov, aspect, nearClip, farClip, viewMatrix);

	auto drawCommand = [&](const RenderCommand3D& cmd) {
		if (cmd.staticMesh)
		{
			render::Renderer3D::DrawStaticMesh(cmd.staticMesh, cmd.transform, cmd.wireframe);
		}
		else
		{
			render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
		}
	};

	for (auto& [key, transforms] : m_instancedOpaque)
	{
		render::Renderer3D::DrawStaticMeshInstanced(key.first, transforms, key.second);
	}

	for (const auto& cmd : m_opaqueQueue)
	{
		drawCommand(cmd);
	}

	if (!m_transparentQueue.empty() || !m_instancedTransparent.empty())
	{
		render::Renderer3D::SetDepthMask(false);
		render::Renderer3D::SetCullMode(render::CullMode::Front);

		for (auto& [key, pairs] : m_instancedTransparent)
		{
			std::sort(std::execution::par_unseq, pairs.begin(), pairs.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
			std::vector<glm::mat4> sortedTransforms;
			sortedTransforms.reserve(pairs.size());
			for (const auto& val : pairs | std::views::values)
			{
				sortedTransforms.push_back(val);
			}

			render::Renderer3D::DrawStaticMeshInstanced(key.first, sortedTransforms, key.second);
		}

		std::sort(std::execution::par_unseq, m_transparentQueue.begin(), m_transparentQueue.end(), [](const RenderCommand3D& a, const RenderCommand3D& b) {
			return a.distanceToCamera > b.distanceToCamera;
		});
		for (const auto& cmd : m_transparentQueue)
		{
			drawCommand(cmd);
		}

		render::Renderer3D::SetCullMode(render::CullMode::Back);
		for (auto& [key, pairs] : m_instancedTransparent)
		{
			std::vector<glm::mat4> sortedTransforms;
			sortedTransforms.reserve(pairs.size());
			for (const auto& val : pairs | std::views::values)
			{
				sortedTransforms.push_back(val);
			}

			render::Renderer3D::DrawStaticMeshInstanced(key.first, sortedTransforms, key.second);
		}

		for (const auto& cmd : m_transparentQueue)
		{
			drawCommand(cmd);
		}

		render::Renderer3D::SetDepthMask(true);
	}

	render::Renderer3D::EndScene();
}

} // namespace re::detail
