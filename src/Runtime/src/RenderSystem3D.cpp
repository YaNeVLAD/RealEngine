#include <Runtime/Internal/RenderSystem3D.hpp>

#include <ECS/Scene.hpp>
#include <Render3D/Renderer3D.hpp>
#include <RenderCore/Light.hpp>
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

	for (auto&& [entity, transform, light] : *scene.CreateView<TransformComponent, LightComponent>())
	{
		data.type = static_cast<int>(light.type);
		data.position = { transform.position.x, transform.position.y, transform.position.z };

		const float yawRad = glm::radians(transform.rotation.y);
		const float pitchRad = glm::radians(transform.rotation.x);
		data.direction.x = std::cos(yawRad) * std::cos(pitchRad);
		data.direction.y = std::sin(pitchRad);
		data.direction.z = std::sin(yawRad) * std::cos(pitchRad);
		data.direction = glm::normalize(data.direction);

		data.ambient = { light.ambient.r / 255.f, light.ambient.g / 255.f, light.ambient.b / 255.f };
		data.diffuse = { light.diffuse.r / 255.f, light.diffuse.g / 255.f, light.diffuse.b / 255.f };
		data.specular = { light.specular.r / 255.f, light.specular.g / 255.f, light.specular.b / 255.f };

		data.constant = light.constant;
		data.linear = light.linear;
		data.quadratic = light.quadratic;
		data.cutOff = glm::cos(glm::radians(light.cutOffAngle));
		data.exponent = light.exponent;
		break;
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
			const float yaw = transform.rotation.y;
			const float pitch = transform.rotation.x;

			front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front.y = std::sin(glm::radians(pitch));
			front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			front = glm::normalize(front);

			const glm::vec3 up = { camera.up.x, camera.up.y, camera.up.z };
			data.viewMatrix = glm::lookAt(data.position, data.position + front, up);
			break; // TODO: Add support for multiple cameras
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
	if (width == 0 || height == 0)
	{
		return;
	}

	ProcessTransforms(scene);

	if (m_isStaticDirty)
	{
		RebuildStaticOpaqueBatches(scene);
	}

	const auto light = ExtractLight(scene);
	const auto [position, viewMatrix, fov, nearClip, farClip] = ExtractCamera(scene);
	const auto skybox = ExtractSkybox(scene);

	CollectDynamicAndTransparent(scene, position);

	render::Renderer3D::BeginScene(fov, aspect, nearClip, farClip, viewMatrix);

	render::Renderer3D::SetLight(light);
	render::Renderer3D::SetEnvironment(skybox.irradianceID);

	RenderOpaqueGeometry(position, farClip);
	RenderTransparentGeometry();

	if (skybox.m_cubemapID != 0)
	{
		glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
		render::Renderer3D::DrawSkybox(skybox.m_cubemapID, viewMatrix, projection);
	}

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
		{
			continue;
		}

		MaterialComponent material;
		if (scene.HasComponent<MaterialComponent>(entity))
		{
			material = scene.GetComponent<MaterialComponent>(entity);
		}

		m_flatOpaqueBatch.emplace_back(
			staticComp.mesh.get(),
			staticComp.wireframe,
			0.0f,
			transform.modelMatrix,
			material);
	}

	std::sort(std::execution::par_unseq, m_flatOpaqueBatch.begin(), m_flatOpaqueBatch.end(), [](const auto& a, const auto& b) {
		if (a.mesh != b.mesh)
		{
			return a.mesh < b.mesh;
		}
		if (a.wireframe != b.wireframe)
		{
			return a.wireframe < b.wireframe;
		}

		return a.material < b.material;
	});

	if (!m_flatOpaqueBatch.empty())
	{
		auto currentMesh = m_flatOpaqueBatch[0].mesh;
		auto currentWireframe = m_flatOpaqueBatch[0].wireframe;
		auto currentMat = m_flatOpaqueBatch[0].material;
		OpaqueBatchCache currentBatch{ currentMesh, currentWireframe, currentMat, {} };

		for (const auto& item : m_flatOpaqueBatch)
		{
			if (item.mesh != currentMesh || item.wireframe != currentWireframe || item.material != currentMat)
			{
				m_cachedOpaqueBatches.emplace_back(currentBatch);
				currentMesh = item.mesh;
				currentWireframe = item.wireframe;
				currentMat = item.material;
				currentBatch = { currentMesh, currentWireframe, currentMat, {} };
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

		MaterialComponent material;
		if (scene.HasComponent<MaterialComponent>(entity))
		{
			material = scene.GetComponent<MaterialComponent>(entity);
		}

		float dist = glm::distance(camPos, glm::vec3(transform.modelMatrix[3]));
		m_flatTransparentBatch.emplace_back(
			staticComp.mesh.get(),
			staticComp.wireframe,
			dist,
			transform.modelMatrix,
			material);
	}

	for (auto&& [entity, transform, dynComp] : *scene.CreateView<TransformComponent, DynamicMeshComponent3D>())
	{
		if (dynComp.vertices.empty() || dynComp.indices.empty())
		{
			continue;
		}

		MaterialComponent material;
		if (scene.HasComponent<MaterialComponent>(entity))
		{
			material = scene.GetComponent<MaterialComponent>(entity);
		}

		RenderCommand3D cmd;
		cmd.transform = transform.modelMatrix;
		cmd.vertices = &dynComp.vertices;
		cmd.indices = &dynComp.indices;
		cmd.wireframe = dynComp.wireframe;
		cmd.material = material;

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

void RenderSystem3D::RenderOpaqueGeometry(const glm::vec3& camPos, float farClip)
{
	std::uint32_t batchIndex = 0;

	render::Renderer3D::SetCullMode(render::CullMode::Back);
	for (const auto& [mesh, wireframe, material, transforms] : m_cachedOpaqueBatches)
	{
		render::Renderer3D::SetMaterial(material.data);
		render::Renderer3D::DrawStaticMeshGPUCulled(batchIndex++, mesh, transforms, 2.0f, camPos, wireframe, farClip);
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
	auto currentMaterial = batch[0].material;

	for (const auto& item : batch)
	{
		if (item.mesh != currentMesh || item.wireframe != currentWireframe || item.material != currentMaterial)
		{
			render::Renderer3D::SetMaterial(currentMaterial.data);
			render::Renderer3D::DrawStaticMeshInstanced(currentMesh, m_instanceBuffer, currentWireframe);
			m_instanceBuffer.clear();

			currentMesh = item.mesh;
			currentWireframe = item.wireframe;
			currentMaterial = item.material;
		}
		m_instanceBuffer.push_back(item.transform);
	}

	render::Renderer3D::SetMaterial(currentMaterial.data);
	render::Renderer3D::DrawStaticMeshInstanced(currentMesh, m_instanceBuffer, currentWireframe);
}

void RenderSystem3D::ExecuteRenderCommand(const RenderCommand3D& cmd)
{
	render::Renderer3D::SetMaterial(cmd.material.data);

	if (cmd.staticMesh)
	{
		render::Renderer3D::DrawStaticMesh(cmd.staticMesh, cmd.transform, cmd.wireframe);
	}
	else
	{
		render::Renderer3D::DrawMesh(*cmd.vertices, *cmd.indices, cmd.transform, cmd.wireframe);
	}
}

SkyboxComponent RenderSystem3D::ExtractSkybox(ecs::Scene& scene)
{
	SkyboxComponent defaultSkybox;

	for (auto&& [entity, skybox] : *scene.CreateView<SkyboxComponent>())
	{
		if (skybox.m_hdrTexture && skybox.m_cubemapID == 0)
		{
			skybox.m_cubemapID = render::Renderer3D::CreateCubemapFromHDR(skybox.m_hdrTexture.get());
			skybox.irradianceID = render::Renderer3D::CreateIrradianceMap(skybox.m_cubemapID);
		}

		return skybox;
	}

	return defaultSkybox;
}

} // namespace re::detail