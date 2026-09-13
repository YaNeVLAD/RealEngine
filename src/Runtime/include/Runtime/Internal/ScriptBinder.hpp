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

	static bool Entity_GetComponentData_Impl(std::uint64_t entityID, scripting::ComponentDataKind kind, void* outData, std::uint32_t maxBytes, std::uint32_t* outBytes);
	static bool Entity_SetComponentData_Impl(std::uint64_t entityID, scripting::ComponentDataKind kind, const void* data, std::uint32_t bytes);

	static bool Input_IsKeyDown_Impl(int key);
	static bool Input_IsMouseButtonDown_Impl(int button);
};

} // namespace re::runtime