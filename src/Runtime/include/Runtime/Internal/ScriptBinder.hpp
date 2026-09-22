#pragma once

#include <Scripting/Interface/EngineApiPointers.hpp>
#include <ECS/Scene.hpp>

namespace re::runtime
{

class ScriptBinder
{
public:
	static void SetActiveScene(ecs::Scene* scene);

	static scripting::EngineApiPointers CreateApiPointers();

private:
	inline static ecs::Scene* s_ActiveScene = nullptr;

	static void NativeLog_Impl(const char* message);
	static void OnScriptError_Impl(const char* message);

	static std::uint64_t Scene_CreateEntity_Impl();
	static bool Scene_IsEntityValid_Impl(std::uint64_t entityID);
	static void Scene_DestroyEntity_Impl(std::uint64_t entityID);

	static std::uint64_t Entity_GetComponentMask_Impl(std::uint64_t entityID);
	static bool Entity_AddComponent_Impl(std::uint64_t entityID, scripting::ReflectedComponentId component);
	static bool Entity_RemoveComponent_Impl(std::uint64_t entityID, scripting::ReflectedComponentId component);
	static std::int32_t Component_GetFieldCount_Impl(scripting::ReflectedComponentId component);
	static bool Component_GetFieldInfo_Impl(scripting::ReflectedComponentId component, std::int32_t fieldIndex, scripting::ComponentFieldInfo* outInfo);
	static bool Entity_GetFieldData_Impl(std::uint64_t entityID, scripting::ReflectedComponentId component, std::int32_t fieldIndex, void* outData, std::uint32_t maxBytes, std::uint32_t* outBytes);
	static bool Entity_SetFieldData_Impl(std::uint64_t entityID, scripting::ReflectedComponentId component, std::int32_t fieldIndex, const void* data, std::uint32_t bytes);

	static std::uint32_t Scene_GetEntityCount_Impl();
	static bool Scene_GetEntities_Impl(std::uint64_t* outIds, std::uint32_t capacity, std::uint32_t* outCount);
	static std::uint32_t Scene_ClearEntities_Impl();
	static bool Entity_GetName_Impl(std::uint64_t entityID, char* outName, std::uint32_t capacity, std::uint32_t* outBytes);
	static bool Entity_SetName_Impl(std::uint64_t entityID, const char* nameUtf8);
	static std::uint64_t Scene_SpawnPrimitive_Impl(std::int32_t kind, std::uint32_t rgba);

	static bool Input_IsKeyDown_Impl(int key);
	static bool Input_IsMouseButtonDown_Impl(int button);
};

} // namespace re::runtime