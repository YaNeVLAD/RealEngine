#include <Runtime/Internal/ScriptBinder.hpp>

#include <Core/Logger.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>
#include <Scripting/CSharp/DotNetInterop.hpp>

#include <algorithm>
#include <cstring>
#include <string_view>

// ReSharper disable CppDeclaratorNeverUsed
// ReSharper disable CppDFAUnreachableFunctionCall
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

using FieldGetter = bool (*)(ecs::Scene* scene, ecs::Entity entity, void* outData);
using FieldSetter = bool (*)(ecs::Scene* scene, ecs::Entity entity, const void* data);

template <typename TComp, float TComp::* Member>
bool GetFloatField(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	*static_cast<float*>(outData) = scene->GetComponent<TComp>(entity).*Member;

	return true;
}

template <typename TComp, float TComp::* Member>
bool SetFloatField(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	scene->GetComponent<TComp>(entity).*Member = *static_cast<const float*>(data);

	return true;
}

template <typename TComp, bool TComp::* Member>
bool GetBoolField(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	*static_cast<std::int32_t*>(outData) = scene->GetComponent<TComp>(entity).*Member ? 1 : 0;

	return true;
}

template <typename TComp, bool TComp::* Member>
bool SetBoolField(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	scene->GetComponent<TComp>(entity).*Member = *static_cast<const std::int32_t*>(data) != 0;

	return true;
}

template <typename TComp, std::int32_t TComp::* Member>
bool GetIntField(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	*static_cast<std::int32_t*>(outData) = scene->GetComponent<TComp>(entity).*Member;

	return true;
}

template <typename TComp, std::int32_t TComp::* Member>
bool SetIntField(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	scene->GetComponent<TComp>(entity).*Member = *static_cast<const std::int32_t*>(data);

	return true;
}

template <typename TComp, Vector2f TComp::* Member>
bool GetFloat2Field(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	const auto& v = scene->GetComponent<TComp>(entity).*Member;
	std::memcpy(outData, v.Data(), sizeof(v));

	return true;
}

template <typename TComp, Vector2f TComp::* Member>
bool SetFloat2Field(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	std::memcpy((scene->GetComponent<TComp>(entity).*Member).Data(), data, sizeof(Vector2f));

	return true;
}

template <typename TComp, Vector3f TComp::* Member>
bool GetFloat3Field(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	const auto& v = scene->GetComponent<TComp>(entity).*Member;
	std::memcpy(outData, v.Data(), sizeof(v));

	return true;
}

template <typename TComp, Vector3f TComp::* Member>
bool SetFloat3Field(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	std::memcpy((scene->GetComponent<TComp>(entity).*Member).Data(), data, sizeof(Vector3f));

	return true;
}

template <typename TComp, Color TComp::* Member>
bool GetColorField(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	const auto& c = scene->GetComponent<TComp>(entity).*Member;
	std::memcpy(outData, c.Data(), sizeof(c));

	return true;
}

template <typename TComp, Color TComp::* Member>
bool SetColorField(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<TComp>(entity))
	{
		return false;
	}
	std::memcpy((scene->GetComponent<TComp>(entity).*Member).Data(), data, sizeof(Color));

	return true;
}

bool GetLightTypeField(ecs::Scene* scene, const ecs::Entity entity, void* outData)
{
	if (!scene->HasComponent<LightComponent>(entity))
	{
		return false;
	}
	*static_cast<std::int32_t*>(outData) = static_cast<std::int32_t>(scene->GetComponent<LightComponent>(entity).type);

	return true;
}

bool SetLightTypeField(ecs::Scene* scene, const ecs::Entity entity, const void* data)
{
	if (!scene->HasComponent<LightComponent>(entity))
	{
		return false;
	}
	const auto value = *static_cast<const std::int32_t*>(data);
	if (value < 0 || value > 2)
	{
		return false;
	}
	scene->GetComponent<LightComponent>(entity).type = static_cast<LightType>(value);

	return true;
}

struct FieldDesc
{
	const char* name;
	scripting::FieldType type;
	std::uint32_t size;
	FieldGetter get;
	FieldSetter set;
};

struct ComponentDesc
{
	bool (*has)(ecs::Scene* scene, ecs::Entity entity);
	bool (*add)(ecs::Scene* scene, ecs::Entity entity);
	bool (*remove)(ecs::Scene* scene, ecs::Entity entity);
	void (*markDirty)(ecs::Scene* scene, ecs::Entity entity);
	const FieldDesc* fields;
	std::int32_t fieldCount;
};

template <typename TComp>
bool HasComp(ecs::Scene* scene, const ecs::Entity entity)
{
	return scene->HasComponent<TComp>(entity);
}

template <typename TComp>
bool AddComp(ecs::Scene* scene, const ecs::Entity entity)
{
	scene->AddComponent<TComp>(entity, TComp{});
	return true;
}

template <typename TComp>
bool RemoveComp(ecs::Scene* scene, const ecs::Entity entity)
{
	scene->RemoveComponent<TComp>(entity);
	return true;
}

template <typename TComp>
bool RemoveRenderComp(ecs::Scene* scene, const ecs::Entity entity)
{
	scene->RemoveComponent<TComp>(entity);
	if (scene->IsRegistered<Dirty<TComp>>() && scene->HasComponent<Dirty<TComp>>(entity))
	{
		scene->RemoveComponent<Dirty<TComp>>(entity);
	}
	return true;
}

template <typename TComp>
void MarkDirtyComp(ecs::Scene* scene, const ecs::Entity entity)
{
	scene->MakeDirty<TComp>(entity);
}

#include "bridge/ReflectionTables.gen.inc"

bool EnsureRegistered(ecs::Scene* scene)
{
	if (!scene)
	{
		return false;
	}

	EnsureComponentRegistered<NameComponent>(scene);

	return RegisterReflectedComponents(scene);
}

const ComponentDesc* FindComponentDesc(const scripting::ReflectedComponentId id)
{
	const auto index = static_cast<std::int32_t>(id);
	if (index < 0 || index >= kReflectedComponentCount)
	{
		return nullptr;
	}
	return &kReflectedComponents[index];
}

const FieldDesc* FindFieldDesc(const ComponentDesc* comp, const std::int32_t fieldIndex)
{
	if (!comp || fieldIndex < 0 || fieldIndex >= comp->fieldCount)
	{
		return nullptr;
	}
	return &comp->fields[fieldIndex];
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

		.Input_IsKeyDown = &Input_IsKeyDown_Impl,
		.Input_IsMouseButtonDown = &Input_IsMouseButtonDown_Impl,

		.Entity_GetComponentMask = &Entity_GetComponentMask_Impl,
		.Entity_AddComponent = &Entity_AddComponent_Impl,
		.Entity_RemoveComponent = &Entity_RemoveComponent_Impl,
		.Entity_GetFieldData = &Entity_GetFieldData_Impl,
		.Entity_SetFieldData = &Entity_SetFieldData_Impl,
		.Entity_GetName = &Entity_GetName_Impl,
		.Entity_SetName = &Entity_SetName_Impl,

		.Component_GetFieldCount = &Component_GetFieldCount_Impl,
		.Component_GetFieldInfo = &Component_GetFieldInfo_Impl,

		.Scene_CreateEntity = &Scene_CreateEntity_Impl,
		.Scene_IsEntityValid = &Scene_IsEntityValid_Impl,
		.Scene_DestroyEntity = &Scene_DestroyEntity_Impl,
		.Scene_GetEntityCount = &Scene_GetEntityCount_Impl,
		.Scene_GetEntities = &Scene_GetEntities_Impl,
		.Scene_ClearEntities = &Scene_ClearEntities_Impl,
		.Scene_SpawnPrimitive = &Scene_SpawnPrimitive_Impl,
		.Scene_ConfirmChanges = &Scene_ConfirmChanges_Impl,
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

bool ScriptBinder::Input_IsKeyDown_Impl(const int key)
{
	return Keyboard::IsKeyPressed(static_cast<Keyboard::Key>(key));
}

bool ScriptBinder::Input_IsMouseButtonDown_Impl(const int button)
{
	return Mouse::IsButtonPressed(static_cast<Mouse::Button>(button));
}

std::uint64_t ScriptBinder::Entity_GetComponentMask_Impl(const std::uint64_t entityID)
{
	if (!EnsureRegistered(s_ActiveScene))
	{
		return 0;
	}

	const auto entity = ToEntity(entityID);
	if (!s_ActiveScene->IsValid(entity))
	{
		return 0;
	}

	std::uint64_t mask = 0;
	for (std::int32_t i = 0; i < static_cast<std::int32_t>(scripting::ReflectedComponentId::Count); ++i)
	{
		if (const auto* desc = FindComponentDesc(static_cast<scripting::ReflectedComponentId>(i)); desc && desc->has(s_ActiveScene, entity))
		{
			mask |= 1ULL << i;
		}
	}

	return mask;
}

bool ScriptBinder::Entity_AddComponent_Impl(const std::uint64_t entityID, const scripting::ReflectedComponentId component)
{
	if (!EnsureRegistered(s_ActiveScene))
	{
		return false;
	}

	const auto* desc = FindComponentDesc(component);
	const auto entity = ToEntity(entityID);
	if (!desc || !s_ActiveScene->IsValid(entity))
	{
		return false;
	}

	const bool ok = desc->add(s_ActiveScene, entity);
	if (ok && desc->markDirty)
	{
		desc->markDirty(s_ActiveScene, entity);
	}

	return ok;
}

bool ScriptBinder::Entity_RemoveComponent_Impl(const std::uint64_t entityID, const scripting::ReflectedComponentId component)
{
	if (!EnsureRegistered(s_ActiveScene))
	{
		return false;
	}

	const auto* desc = FindComponentDesc(component);
	const auto entity = ToEntity(entityID);
	if (!desc || !s_ActiveScene->IsValid(entity))
	{
		return false;
	}

	return desc->remove(s_ActiveScene, entity);
}

std::int32_t ScriptBinder::Component_GetFieldCount_Impl(const scripting::ReflectedComponentId component)
{
	if (const auto* desc = FindComponentDesc(component))
	{
		return desc->fieldCount;
	}

	return -1;
}

bool ScriptBinder::Component_GetFieldInfo_Impl(const scripting::ReflectedComponentId component, const std::int32_t fieldIndex, scripting::ComponentFieldInfo* outInfo)
{
	const auto* desc = FindComponentDesc(component);
	const auto* field = FindFieldDesc(desc, fieldIndex);
	if (!field || !outInfo)
	{
		return false;
	}

	std::strncpy(outInfo->name, field->name, sizeof(outInfo->name) - 1);
	outInfo->name[sizeof(outInfo->name) - 1] = '\0';
	outInfo->type = field->type;
	outInfo->size = field->size;

	return true;
}

bool ScriptBinder::Entity_GetFieldData_Impl(
	const std::uint64_t entityID,
	const scripting::ReflectedComponentId component,
	const std::int32_t fieldIndex,
	void* outData,
	const std::uint32_t maxBytes,
	std::uint32_t* outBytes)
{
	if (!EnsureRegistered(s_ActiveScene) || !outData || !outBytes)
	{
		return false;
	}

	const auto* field = FindFieldDesc(FindComponentDesc(component), fieldIndex);
	const auto entity = ToEntity(entityID);
	if (!field || !s_ActiveScene->IsValid(entity) || maxBytes < field->size)
	{
		return false;
	}

	if (!field->get(s_ActiveScene, entity, outData))
	{
		return false;
	}
	*outBytes = field->size;

	return true;
}

bool ScriptBinder::Entity_SetFieldData_Impl(
	const std::uint64_t entityID,
	const scripting::ReflectedComponentId component,
	const std::int32_t fieldIndex,
	const void* data,
	const std::uint32_t bytes)
{
	if (!EnsureRegistered(s_ActiveScene) || !data)
	{
		return false;
	}

	const auto* compDesc = FindComponentDesc(component);
	const auto* field = FindFieldDesc(compDesc, fieldIndex);
	const auto entity = ToEntity(entityID);
	if (!field || !s_ActiveScene->IsValid(entity) || bytes != field->size)
	{
		return false;
	}

	if (!field->set(s_ActiveScene, entity, data))
	{
		return false;
	}
	if (compDesc->markDirty)
	{
		compDesc->markDirty(s_ActiveScene, entity);
	}

	return true;
}

std::uint32_t ScriptBinder::Scene_GetEntityCount_Impl()
{
	if (!s_ActiveScene)
	{
		return 0;
	}

	return static_cast<std::uint32_t>(s_ActiveScene->GetAllEntities().size());
}

bool ScriptBinder::Scene_GetEntities_Impl(std::uint64_t* outIds, const std::uint32_t capacity, std::uint32_t* outCount)
{
	if (!s_ActiveScene || !outIds || !outCount)
	{
		return false;
	}

	const auto entities = s_ActiveScene->GetAllEntities();
	const auto total = static_cast<std::uint32_t>(entities.size());
	const auto count = std::min(total, capacity);
	for (std::uint32_t i = 0; i < count; ++i)
	{
		outIds[i] = entities[i].Id();
	}
	*outCount = total;

	return true;
}

std::uint32_t ScriptBinder::Scene_ClearEntities_Impl()
{
	if (!s_ActiveScene)
	{
		return 0;
	}

	const auto entities = s_ActiveScene->GetAllEntities();
	for (const auto entity : entities)
	{
		s_ActiveScene->DestroyEntity(entity);
	}

	return static_cast<std::uint32_t>(entities.size());
}

bool ScriptBinder::Entity_GetName_Impl(const std::uint64_t entityID, char* outName, const std::uint32_t capacity, std::uint32_t* outBytes)
{
	if (!EnsureRegistered(s_ActiveScene) || !outBytes)
	{
		return false;
	}

	const auto entity = ToEntity(entityID);
	if (!s_ActiveScene->IsValid(entity) || !s_ActiveScene->HasComponent<NameComponent>(entity))
	{
		*outBytes = 0;
		return false;
	}

	const std::string bytes = s_ActiveScene->GetComponent<NameComponent>(entity).name.ToString();
	const auto needed = static_cast<std::uint32_t>(bytes.size());
	*outBytes = needed;
	if (!outName || capacity == 0)
	{
		return false;
	}
	if (capacity < needed)
	{
		return false;
	}

	std::memcpy(outName, bytes.data(), needed);

	return true;
}

bool ScriptBinder::Entity_SetName_Impl(const std::uint64_t entityID, const char* nameUtf8)
{
	if (!EnsureRegistered(s_ActiveScene) || !nameUtf8)
	{
		return false;
	}

	const auto entity = ToEntity(entityID);
	if (!s_ActiveScene->IsValid(entity))
	{
		return false;
	}

	if (nameUtf8[0] == '\0')
	{
		s_ActiveScene->RemoveComponent<NameComponent>(entity);
		return true;
	}

	if (!s_ActiveScene->HasComponent<NameComponent>(entity))
	{
		s_ActiveScene->AddComponent<NameComponent>(entity, NameComponent{});
	}
	s_ActiveScene->GetComponent<NameComponent>(entity).name = String(std::string_view{ nameUtf8 });

	return true;
}

std::uint64_t ScriptBinder::Scene_SpawnPrimitive_Impl(const std::int32_t kind, const std::uint32_t rgba)
{
	if (!EnsureRegistered(s_ActiveScene))
	{
		return ecs::Entity::INVALID_ID.Id();
	}

	const Color color{ rgba };

	std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> mesh;
	switch (static_cast<scripting::PrimitiveKind>(kind))
	{
	case scripting::PrimitiveKind::Cube:
		mesh = detail::PrimitiveBuilder::CreateCube(color);
		break;
	case scripting::PrimitiveKind::Sphere:
		mesh = detail::PrimitiveBuilder::CreateSphere(color);
		break;
	default:
		return ecs::Entity::INVALID_ID.Id();
	}

	Material material;
	material.metallicFactor = 0.f;
	material.albedoColor = color;
	material.emissionColor = color;

	const auto entity = s_ActiveScene->CreateEntity()
							.Add<Dirty<TransformComponent>>()
							.Add<TransformComponent>()
							.Add<detail::OpaqueTag>()
							.Add<StaticMeshComponent3D>(mesh.first, mesh.second)
							.Add<MaterialComponent>(material)
							.GetEntity();

	return entity.Id();
}

void ScriptBinder::Scene_ConfirmChanges_Impl()
{
	if (s_ActiveScene)
	{
		s_ActiveScene->ConfirmChanges();
	}
}

} // namespace re::runtime