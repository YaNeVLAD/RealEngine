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
using namespace re;

struct CameraData
{
	glm::vec3 position{ 0.f, 0.f, 0.f };
	glm::mat4 viewMatrix{ 1.0f };
	float fov = 45.0f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;
};

struct LightData
{
	glm::vec3 direction{ -0.5f, -1.0f, -0.5f };
	Color color = Color::White;
	float ambient = 0.3f;
};

glm::mat4 CalculateModelMatrix(TransformComponent const& transform)
{
	glm::mat4 result = glm::translate(glm::mat4(1.0f), glm::vec3(transform.position.x, transform.position.y, transform.position.z));

	result *= glm::eulerAngleYXZ(
		glm::radians(transform.rotation.y),
		glm::radians(transform.rotation.x),
		glm::radians(transform.rotation.z));

	result = glm::scale(result, glm::vec3(transform.scale.x, transform.scale.y, transform.scale.z));

	return result;
}

LightData ExtractLight(ecs::Scene& scene)
{
	LightData data;
	for (auto&& [entity, transform, light] : *scene.CreateView<TransformComponent, DirectionalLightComponent>())
	{
		const float yaw = transform.rotation.y;
		const float pitch = transform.rotation.x;

		data.direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
		data.direction.y = std::sin(glm::radians(pitch));
		data.direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
		data.direction = glm::normalize(data.direction);

		data.color = light.color;
		data.ambient = light.ambientIntensity;

		break; // TODO: Add support for multiple light sources
	}

	return data;
}

CameraData ExtractCamera(ecs::Scene& scene)
{
	CameraData data;
	for (auto&& [entity, transform, camera] : *scene.CreateView<TransformComponent, CameraComponent>())
	{
		if (camera.isPrimal)
		{
			data.position = { transform.position.x, transform.position.y, transform.position.z };
			data.fov = camera.fov;
			data.nearClip = camera.nearClip;
			data.farClip = camera.farClip;

			glm::vec3 front;
			float yaw = transform.rotation.y;
			float pitch = transform.rotation.x;

			front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front.y = std::sin(glm::radians(pitch));
			front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front = glm::normalize(front);

			glm::vec3 up = { camera.up.x, camera.up.y, camera.up.z };
			data.viewMatrix = glm::lookAt(data.position, data.position + front, up);
			break;
		}
	}

	return data;
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
	auto [width, height] = m_window.Size();
	const float aspect = static_cast<float>(width) / static_cast<float>(height);

	ProcessTransforms(scene);

	if (m_isStaticDirty)
	{
		RebuildStaticOpaqueBatches(scene);
	}

	const auto [direction, color, ambient] = ExtractLight(scene);
	const auto [position, viewMatrix, fov, nearClip, farClip] = ExtractCamera(scene);

	CollectDynamicAndTransparent(scene, position);

	render::Renderer3D::BeginScene(fov, aspect, nearClip, farClip, viewMatrix);

	render::Renderer3D::SetDirectionalLight(direction, color, ambient);

	RenderOpaqueGeometry(position);
	RenderTransparentGeometry();

	render::Renderer3D::EndScene();
}

void RenderSystem3D::ProcessTransforms(ecs::Scene& scene)
{
	std::vector<ecs::Entity> processedEntities;
	for (auto&& [entity, transform, _] : *scene.CreateView<TransformComponent, DirtyTag<TransformComponent>>())
	{
		transform.modelMatrix = CalculateModelMatrix(transform);
		processedEntities.emplace_back(entity);

		if (scene.HasComponent<StaticMeshComponent3D>(entity))
		{
			m_isStaticDirty = true;
		}
	}

	for (const auto& entity : processedEntities)
	{
		scene.RemoveComponent<DirtyTag<TransformComponent>>(entity);
	}
}

void RenderSystem3D::RebuildStaticOpaqueBatches(ecs::Scene& scene)
{
	render::Renderer3D::ResetCullingCache();

	m_flatOpaqueBatch.clear();
	m_cachedOpaqueBatches.clear();

	for (auto&& [entity, transform, staticComp, _] : *scene.CreateView<TransformComponent, StaticMeshComponent3D, OpaqueTag>())
	{
		if (!staticComp.mesh)
			continue;
		m_flatOpaqueBatch.emplace_back(staticComp.mesh.get(), staticComp.wireframe, 0.0f, transform.modelMatrix);
	}

	std::sort(std::execution::par_unseq, m_flatOpaqueBatch.begin(), m_flatOpaqueBatch.end(), [](const auto& a, const auto& b) {
		if (a.mesh != b.mesh)
			return a.mesh < b.mesh;
		return a.wireframe < b.wireframe;
	});

	if (!m_flatOpaqueBatch.empty())
	{
		auto currentMesh = m_flatOpaqueBatch[0].mesh;
		auto currentWireframe = m_flatOpaqueBatch[0].wireframe;
		OpaqueBatchCache currentBatch{ currentMesh, currentWireframe, {} };

		for (const auto& item : m_flatOpaqueBatch)
		{
			if (item.mesh != currentMesh || item.wireframe != currentWireframe)
			{
				m_cachedOpaqueBatches.emplace_back(currentBatch);
				currentMesh = item.mesh;
				currentWireframe = item.wireframe;
				currentBatch = { currentMesh, currentWireframe, {} };
			}
			currentBatch.transforms.emplace_back(item.transform);
		}
		m_cachedOpaqueBatches.emplace_back(currentBatch);
	}

	m_isStaticDirty = false;
}

void RenderSystem3D::CollectDynamicAndTransparent(ecs::Scene& scene, const glm::vec3& camPos)
{
	m_opaqueQueue.clear();
	m_transparentQueue.clear();
	m_flatTransparentBatch.clear();

	for (auto&& [entity, transform, staticComp, _] : *scene.CreateView<TransformComponent, StaticMeshComponent3D, TransparentTag>())
	{
		if (!staticComp.mesh)
		{
			continue;
		}

		float dist = glm::distance(camPos, glm::vec3(transform.modelMatrix[3]));
		m_flatTransparentBatch.emplace_back(staticComp.mesh.get(), staticComp.wireframe, dist, transform.modelMatrix);
	}

	for (auto&& [entity, transform, dynComp] : *scene.CreateView<TransformComponent, DynamicMeshComponent3D>())
	{
		if (dynComp.vertices.empty() || dynComp.indices.empty())
		{
			continue;
		}

		RenderCommand3D cmd;
		cmd.transform = transform.modelMatrix;
		cmd.vertices = &dynComp.vertices;
		cmd.indices = &dynComp.indices;
		cmd.wireframe = dynComp.wireframe;

		if (dynComp.vertices[0].color.a < 255)
		{
			cmd.distanceToCamera = glm::distance(camPos, glm::vec3(cmd.transform[3]));
			m_transparentQueue.push_back(cmd);
		}
		else
		{
			m_opaqueQueue.push_back(cmd);
		}
	}
}

void RenderSystem3D::RenderOpaqueGeometry(const glm::vec3& camPos)
{
	std::uint32_t batchIndex = 0;

	render::Renderer3D::SetCullMode(render::CullMode::Back);
	for (const auto& [mesh, wireframe, transforms] : m_cachedOpaqueBatches)
	{
		render::Renderer3D::DrawStaticMeshGPUCulled(batchIndex++, mesh, transforms, 2.0f, camPos, wireframe);
	}

	for (const auto& cmd : m_opaqueQueue)
	{
		ExecuteRenderCommand(cmd);
	}
}

void RenderSystem3D::RenderTransparentGeometry()
{
	if (m_transparentQueue.empty() && m_flatTransparentBatch.empty())
	{
		return;
	}

	render::Renderer3D::SetDepthMask(false);

	std::sort(std::execution::par_unseq, m_flatTransparentBatch.begin(), m_flatTransparentBatch.end(), [](const auto& a, const auto& b) {
		if (a.mesh != b.mesh)
		{
			return a.mesh < b.mesh;
		}
		if (a.wireframe != b.wireframe)
		{
			return a.wireframe < b.wireframe;
		}

		return a.distance > b.distance;
	});

	std::sort(std::execution::par_unseq, m_transparentQueue.begin(), m_transparentQueue.end(), [](const RenderCommand3D& a, const RenderCommand3D& b) {
		return a.distanceToCamera > b.distanceToCamera;
	});

	render::Renderer3D::SetCullMode(render::CullMode::Front);
	DrawFlatBatch(m_flatTransparentBatch);
	for (const auto& cmd : m_transparentQueue)
	{
		ExecuteRenderCommand(cmd);
	}

	render::Renderer3D::SetCullMode(render::CullMode::Back);
	DrawFlatBatch(m_flatTransparentBatch);
	for (const auto& cmd : m_transparentQueue)
	{
		ExecuteRenderCommand(cmd);
	}

	render::Renderer3D::SetDepthMask(true);
}

void RenderSystem3D::DrawFlatBatch(const std::vector<StaticBatchItem>& batch)
{
	if (batch.empty())
	{
		return;
	}

	m_instanceBuffer.clear();
	auto currentMesh = batch[0].mesh;
	auto currentWireframe = batch[0].wireframe;

	for (const auto& item : batch)
	{
		if (item.mesh != currentMesh || item.wireframe != currentWireframe)
		{
			render::Renderer3D::DrawStaticMeshInstanced(currentMesh, m_instanceBuffer, currentWireframe);
			m_instanceBuffer.clear();
			currentMesh = item.mesh;
			currentWireframe = item.wireframe;
		}
		m_instanceBuffer.push_back(item.transform);
	}
	render::Renderer3D::DrawStaticMeshInstanced(currentMesh, m_instanceBuffer, currentWireframe);
}

void RenderSystem3D::ExecuteRenderCommand(const RenderCommand3D& cmd)
{
	if (cmd.staticMesh)
	{
		render::Renderer3D::DrawStaticMesh(cmd.staticMesh, cmd.transform, cmd.wireframe);
	}
	else
	{
		render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
	}
}

} // namespace re::detail