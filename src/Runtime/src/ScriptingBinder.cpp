#include <Runtime/Internal/ScriptBinder.hpp>

#include <Runtime/Components.hpp>

namespace re::runtime
{

void ScriptBinder::SetActiveScene(ecs::Scene* scene)
{
	s_ActiveScene = scene;
}

scripting::EngineApiPointers ScriptBinder::CreateApiPointers()
{
	return scripting::EngineApiPointers{
		.NativeLog = &NativeLog_Impl,
		.Transform_GetPosition = &Transform_GetPosition_Impl,
		.Transform_SetPosition = &Transform_SetPosition_Impl,
	};
}

void ScriptBinder::NativeLog_Impl(const char* message)
{
	std::cout << "[C#] " << message << std::endl;
}

void ScriptBinder::Transform_GetPosition_Impl(const std::uint64_t entityID, Vector3f* outPosition)
{
	if (const auto entity = ecs::Entity(entityID); s_ActiveScene && s_ActiveScene->IsValid(entity))
	{
		*outPosition = s_ActiveScene->GetComponent<TransformComponent>(entity).position;
	}
}

void ScriptBinder::Transform_SetPosition_Impl(const std::uint64_t entityID, const Vector3f* inPosition)
{
	if (const auto entity = ecs::Entity(entityID); s_ActiveScene && s_ActiveScene->IsValid(entity))
	{
		auto& transform = s_ActiveScene->GetComponent<TransformComponent>(entity);
		transform.position = *inPosition;
		s_ActiveScene->MakeDirty<TransformComponent>(entity);
	}
}

} // namespace re::runtime