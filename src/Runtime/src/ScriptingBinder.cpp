#include <Runtime/Internal/ScriptBinder.hpp>

#include <Core/Logger.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>
#include <Runtime/Components.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>

#include <algorithm>
#include <cstring>

namespace re::runtime
{

namespace
{

ecs::Entity ToEntity(const std::uint64_t entityID)
{
	return ecs::Entity{ entityID };
}

template <typename T>
bool EnsureComponentRegistered(ecs::Scene* scene)
{
	if (scene->IsRegistered<T>())
	{
		return true;
	}

	scene->RegisterComponent<T>();
	return true;
}

bool EnsureRegistered(ecs::Scene* scene)
{
	if (!scene)
	{
		return false;
	}

	EnsureComponentRegistered<TransformComponent>(scene);
	EnsureComponentRegistered<CameraComponent>(scene);
	EnsureComponentRegistered<LightComponent>(scene);

	return true;
}

bool GetTransformSnapshot(ecs::Scene* scene, const ecs::Entity& entity, scripting::TransformComponentData* out)
{
	if (!scene->IsValid(entity) || !scene->HasComponent<TransformComponent>(entity))
	{
		return false;
	}

	const auto& t = scene->GetComponent<TransformComponent>(entity);
	out->position[0] = t.position.x;
	out->position[1] = t.position.y;
	out->position[2] = t.position.z;
	out->rotation[0] = t.rotation.x;
	out->rotation[1] = t.rotation.y;
	out->rotation[2] = t.rotation.z;
	out->scale[0] = t.scale.x;
	out->scale[1] = t.scale.y;
	out->scale[2] = t.scale.z;

	return true;
}

bool SetTransformSnapshot(ecs::Scene* scene, const ecs::Entity& entity, const scripting::TransformComponentData* in)
{
	if (!scene->IsValid(entity))
	{
		return false;
	}

	auto& t = scene->GetComponent<TransformComponent>(entity);
	t.position = Vector3f{ in->position[0], in->position[1], in->position[2] };
	t.rotation = Vector3f{ in->rotation[0], in->rotation[1], in->rotation[2] };
	t.scale = Vector3f{ in->scale[0], in->scale[1], in->scale[2] };
	scene->MakeDirty<TransformComponent>(entity);

	return true;
}

} // namespace

void ScriptBinder::SetActiveScene(ecs::Scene* scene)
{
	s_ActiveScene = scene;
}

scripting::EngineApiPointers ScriptBinder::CreateApiPointers()
{
	return scripting::EngineApiPointers{
		.apiVersion = scripting::EngineApiVersion,
		.NativeLog = &NativeLog_Impl,
		.OnScriptError = &OnScriptError_Impl,
		.Scene_CreateEntity = &Scene_CreateEntity_Impl,
		.Scene_IsEntityValid = &Scene_IsEntityValid_Impl,
		.Scene_DestroyEntity = &Scene_DestroyEntity_Impl,
		.Entity_GetComponentData = &Entity_GetComponentData_Impl,
		.Entity_SetComponentData = &Entity_SetComponentData_Impl,
		.Input_IsKeyDown = &Input_IsKeyDown_Impl,
		.Input_IsMouseButtonDown = &Input_IsMouseButtonDown_Impl,
	};
}

void ScriptBinder::NativeLog_Impl(const char* message)
{
	using namespace re::literals;
	RE_LOG_INFO("C#"_logcat, "{}", message);
}

void ScriptBinder::OnScriptError_Impl(const char* message)
{
	using namespace re::literals;
	RE_LOG_ERROR("C#"_logcat, "{}", message);
}

std::uint64_t ScriptBinder::Scene_CreateEntity_Impl()
{
	if (!EnsureRegistered(s_ActiveScene))
	{
		return ecs::Entity::INVALID_ID.Id();
	}

	const auto entity = s_ActiveScene->CreateEntity()
							.Add<Dirty<TransformComponent>>()
							.Add<TransformComponent>()
							.GetEntity();

	return entity.Id();
}

bool ScriptBinder::Scene_IsEntityValid_Impl(const std::uint64_t entityID)
{
	return s_ActiveScene && s_ActiveScene->IsValid(ToEntity(entityID));
}

void ScriptBinder::Scene_DestroyEntity_Impl(const std::uint64_t entityID)
{
	if (const auto entity = ToEntity(entityID); s_ActiveScene && s_ActiveScene->IsValid(entity))
	{
		s_ActiveScene->DestroyEntity(entity);
	}
}

bool ScriptBinder::Entity_GetComponentData_Impl(
	const std::uint64_t entityID,
	const scripting::ComponentDataKind kind,
	void* outData,
	const std::uint32_t maxBytes,
	std::uint32_t* outBytes)
{
	if (!EnsureRegistered(s_ActiveScene) || !outData || !outBytes)
	{
		return false;
	}

	const auto entity = ToEntity(entityID);

	switch (kind)
	{
	case scripting::ComponentDataKind::Transform: {
		if (maxBytes < sizeof(scripting::TransformComponentData))
		{
			return false;
		}
		scripting::TransformComponentData data{};
		if (!GetTransformSnapshot(s_ActiveScene, entity, &data))
		{
			return false;
		}
		std::memcpy(outData, &data, sizeof(data));
		*outBytes = sizeof(data);
		return true;
	}
	case scripting::ComponentDataKind::Camera: {
		if (!s_ActiveScene->IsValid(entity) || !s_ActiveScene->HasComponent<CameraComponent>(entity) || maxBytes < sizeof(scripting::CameraComponentData))
		{
			return false;
		}
		const auto& cam = s_ActiveScene->GetComponent<CameraComponent>(entity);
		scripting::CameraComponentData data{
			.fov = cam.fov,
			.nearClip = cam.nearClip,
			.farClip = cam.farClip,
			.zoom = cam.zoom,
			.isPrimal = cam.isPrimal ? 1u : 0u,
		};
		std::memcpy(outData, &data, sizeof(data));
		*outBytes = sizeof(data);
		return true;
	}
	case scripting::ComponentDataKind::Light: {
		if (!s_ActiveScene->IsValid(entity) || !s_ActiveScene->HasComponent<LightComponent>(entity) || maxBytes < sizeof(scripting::LightComponentData))
		{
			return false;
		}
		const auto& light = s_ActiveScene->GetComponent<LightComponent>(entity);
		scripting::LightComponentData data{
			.type = static_cast<std::int32_t>(light.type),
			.color = { light.diffuse.r / 255.0f, light.diffuse.g / 255.0f, light.diffuse.b / 255.0f },
			.falloff = light.falloff,
			.cutOffAngle = light.cutOffAngle,
			.ambientIntensity = light.ambientIntensity,
		};
		std::memcpy(outData, &data, sizeof(data));
		*outBytes = sizeof(data);
		return true;
	}
	default:
		return false;
	}
}

bool ScriptBinder::Entity_SetComponentData_Impl(const std::uint64_t entityID, const scripting::ComponentDataKind kind, const void* data, const std::uint32_t bytes)
{
	if (!EnsureRegistered(s_ActiveScene) || !data)
	{
		return false;
	}

	const auto entity = ToEntity(entityID);
	if (!s_ActiveScene->IsValid(entity))
	{
		return false;
	}

	switch (kind)
	{
	case scripting::ComponentDataKind::Transform: {
		if (bytes != sizeof(scripting::TransformComponentData))
		{
			return false;
		}
		if (!s_ActiveScene->HasComponent<TransformComponent>(entity))
		{
			s_ActiveScene->AddComponent<TransformComponent>(entity, TransformComponent{});
		}
		return SetTransformSnapshot(s_ActiveScene, entity, static_cast<const scripting::TransformComponentData*>(data));
	}
	case scripting::ComponentDataKind::Camera: {
		if (bytes != sizeof(scripting::CameraComponentData))
		{
			return false;
		}
		if (!s_ActiveScene->HasComponent<CameraComponent>(entity))
		{
			s_ActiveScene->AddComponent<CameraComponent>(entity, CameraComponent{});
		}
		const auto* camData = static_cast<const scripting::CameraComponentData*>(data);
		auto& cam = s_ActiveScene->GetComponent<CameraComponent>(entity);
		cam.fov = camData->fov;
		cam.nearClip = camData->nearClip;
		cam.farClip = camData->farClip;
		cam.zoom = camData->zoom;
		cam.isPrimal = camData->isPrimal != 0;
		s_ActiveScene->MakeDirty<CameraComponent>(entity);
		return true;
	}
	case scripting::ComponentDataKind::Light: {
		if (bytes != sizeof(scripting::LightComponentData))
		{
			return false;
		}
		if (!s_ActiveScene->HasComponent<LightComponent>(entity))
		{
			s_ActiveScene->AddComponent<LightComponent>(entity, LightComponent{});
		}
		const auto* lightData = static_cast<const scripting::LightComponentData*>(data);
		auto& light = s_ActiveScene->GetComponent<LightComponent>(entity);
		light.type = static_cast<LightType>(lightData->type);
		light.diffuse = Color{
			static_cast<std::uint8_t>(std::clamp(lightData->color[0], 0.0f, 1.0f) * 255.0f),
			static_cast<std::uint8_t>(std::clamp(lightData->color[1], 0.0f, 1.0f) * 255.0f),
			static_cast<std::uint8_t>(std::clamp(lightData->color[2], 0.0f, 1.0f) * 255.0f),
			255,
		};
		light.falloff = lightData->falloff;
		light.cutOffAngle = lightData->cutOffAngle;
		light.ambientIntensity = lightData->ambientIntensity;
		s_ActiveScene->MakeDirty<LightComponent>(entity);
		return true;
	}
	default:
		return false;
	}
}

bool ScriptBinder::Input_IsKeyDown_Impl(const int key)
{
	return Keyboard::IsKeyPressed(static_cast<Keyboard::Key>(key));
}

bool ScriptBinder::Input_IsMouseButtonDown_Impl(const int button)
{
	return Mouse::IsButtonPressed(static_cast<Mouse::Button>(button));
}

} // namespace re::runtime