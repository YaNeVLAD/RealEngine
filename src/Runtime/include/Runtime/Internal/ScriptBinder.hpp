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
	static void Transform_GetPosition_Impl(std::uint64_t entityID, Vector3f* outPosition);
	static void Transform_SetPosition_Impl(std::uint64_t entityID, const Vector3f* inPosition);
};

} // namespace re::runtime