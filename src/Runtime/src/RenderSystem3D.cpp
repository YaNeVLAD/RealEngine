#include <Runtime/Internal/RenderSystem3D.hpp>

#include <Runtime/Components.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

#include <vector>

namespace re::render
{

namespace
{

glm::mat4 CalculateModelMatrix(const TransformComponent& transform)
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

RenderSystem3D::RenderSystem3D(IRenderBackend& backend)
	: m_backend(backend)
{
}

void RenderSystem3D::Update(ecs::Scene& scene, [[maybe_unused]] float dt)
{
	ProcessTransforms(scene);
	ProcessCameras(scene);
	ProcessSkybox(scene);
	ProcessLights(scene);
	SyncRenderEntities(scene);
	CleanupDestroyedEntities(scene);
}

void RenderSystem3D::ProcessTransforms(ecs::Scene& scene)
{
	std::vector<ecs::Entity> processedEntities;

	for (auto&& [entity, transform, _] : *scene.CreateView<TransformComponent, Dirty<TransformComponent>>())
	{
		transform.modelMatrix = CalculateModelMatrix(transform);
		processedEntities.emplace_back(entity);
	}

	for (const auto& entity : processedEntities)
	{
		scene.RemoveComponent<Dirty<TransformComponent>>(entity);
	}
}

void RenderSystem3D::ProcessCameras(ecs::Scene& scene) const
{
	for (auto&& [entity, camera, transform] : *scene.CreateView<CameraComponent, TransformComponent>())
	{
		if (!camera.isPrimal)
		{
			continue;
		}

		CameraDataView cameraView{};
		cameraView.position = Vector3f{ transform.position.x, transform.position.y, transform.position.z };

		glm::mat4 rotationMat(1.0f);
		rotationMat = glm::rotate(rotationMat, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
		rotationMat = glm::rotate(rotationMat, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));

		const auto eye = glm::vec3(transform.position.x, transform.position.y, transform.position.z);
		const auto forward = glm::vec3(rotationMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
		const auto upVec = glm::vec3(rotationMat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

		cameraView.viewMatrix = glm::lookAt(eye, eye + forward, upVec);

		cameraView.fov = camera.fov;
		cameraView.nearClip = std::max(camera.nearClip, 0.01f);
		cameraView.farClip = camera.farClip;
		cameraView.aspect = 16.0f / 9.0f;
		cameraView.iso = camera.iso;

		m_backend.UpdateCamera(cameraView);
		break;
	}
}

void RenderSystem3D::ProcessSkybox(ecs::Scene& scene) const
{
	for (auto&& [entity, skybox] : *scene.CreateView<SkyboxComponent>())
	{
		// Вызываем SetSkybox только если данные обновились (потребуется добавить Dirty тег или проверку стейта)
		if (scene.HasComponent<Dirty<SkyboxComponent>>(entity))
		{
			m_backend.SetSkybox(skybox.m_cubemapID, skybox.m_irradianceID);
		}
	}
}

void RenderSystem3D::ProcessLights(ecs::Scene& scene)
{
	for (auto&& [entity, light, transform] : *scene.CreateView<LightComponent, TransformComponent>())
	{
		LightDataView lightView{};

		switch (light.type)
		{
		case LightType::Directional:
			lightView.type = LightDataView::LightType::Directional;
			break;
		case LightType::Point:
			lightView.type = LightDataView::LightType::Light;
			break;
		case LightType::Spotlight:
			lightView.type = LightDataView::LightType::Spot;
			break;
		}

		lightView.position = glm::vec3(transform.position.x, transform.position.y, transform.position.z);
		glm::vec3 forward = -glm::vec3(transform.modelMatrix[2]);
		lightView.direction = glm::length2(forward) > 0.0001f ? glm::normalize(forward) : glm::vec3(0.0f, -1.0f, 0.0f);

		lightView.color = glm::vec3(light.diffuse.r / 255.0f, light.diffuse.g / 255.0f, light.diffuse.b / 255.0f);
		lightView.intensity = light.exponent;
		lightView.cutOffAngle = light.cutOffAngle;
		lightView.falloff = light.falloff;

		if (auto& handles = m_renderHandles[entity]; handles.lightHandle)
		{
			m_backend.UpdateLight(handles.lightHandle, lightView);
		}
		else
		{
			handles.lightHandle = m_backend.CreateLight(lightView);
		}

		m_backend.SetAmbientLight(light.ambientIntensity * 100000.0f, light.ambient);
	}
}

void RenderSystem3D::SyncRenderEntities(ecs::Scene& scene)
{
	for (auto&& [entity, meshComp, transformComp] : *scene.CreateView<StaticMeshComponent3D, TransformComponent>())
	{
		auto& [meshHandle, lightHandle] = m_renderHandles[entity];

		if (!meshHandle)
		{
			MeshDataView meshView{};
			if (meshComp.mesh)
			{
				meshView.vertices = meshComp.mesh->GetVertices().data();
				meshView.vertexCount = meshComp.mesh->GetVertices().size();
				meshView.vertexStride = sizeof(Vertex);
				meshView.indices = meshComp.mesh->GetIndices().data();
				meshView.indexCount = meshComp.mesh->GetIndices().size();
				meshView.is32BitIndices = true;
			}

			MaterialDataView matView{};
			if (scene.HasComponent<MaterialComponent>(entity))
			{
				const auto& matComp = scene.GetComponent<MaterialComponent>(entity);
				matView.ambient = Vector3f{ matComp.data.ambientColor.r / 255.0f, matComp.data.ambientColor.g / 255.0f, matComp.data.ambientColor.b / 255.0f };
				matView.diffuse = Vector3f{ matComp.data.albedoColor.r / 255.0f, matComp.data.albedoColor.g / 255.0f, matComp.data.albedoColor.b / 255.0f };
				matView.specular = Vector3f{ matComp.data.specularColor.r / 255.0f, matComp.data.specularColor.g / 255.0f, matComp.data.specularColor.b / 255.0f };
				matView.shininess = matComp.data.shininess;
				matView.albedoTexture = matComp.data.albedoMap.get();
			}
			else if (meshComp.mesh)
			{
				matView.diffuse = Vector3f{ 1.0f };
				matView.albedoTexture = meshComp.mesh->GetMaterial().albedoMap.get();
			}

			meshHandle = m_backend.CreateStaticMesh(meshView, matView);
		}

		m_backend.UpdateTransform(meshHandle, transformComp.modelMatrix);
	}
}

void RenderSystem3D::CleanupDestroyedEntities(const ecs::Scene& scene)
{
	for (auto it = m_renderHandles.begin(); it != m_renderHandles.end();)
	{
		const ecs::Entity entity = it->first;
		auto& handles = it->second;

		const bool isEntityValid = scene.IsValid(entity);

		const bool hasMesh = isEntityValid && scene.HasComponent<StaticMeshComponent3D>(entity);
		const bool hasLight = isEntityValid && scene.HasComponent<LightComponent>(entity);

		if (!hasMesh && handles.meshHandle)
		{
			m_backend.DestroyEntity(handles.meshHandle);
			handles.meshHandle = RenderEntityHandle{};
		}

		if (!hasLight && handles.lightHandle)
		{
			m_backend.DestroyEntity(handles.lightHandle);
			handles.lightHandle = RenderEntityHandle{};
		}

		if (handles.IsEmpty())
		{
			it = m_renderHandles.erase(it);
		}
		else
		{
			++it;
		}
	}
}

} // namespace re::render