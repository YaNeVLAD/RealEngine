#include <Runtime/Internal/ScriptBinder.hpp>

#include <Core/Logger.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>
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
		.Input_IsKeyDown = &Input_IsKeyDown_Impl,
		.Input_IsMouseButtonDown = &Input_IsMouseButtonDown_Impl,
	};
}

void ScriptBinder::NativeLog_Impl(const char* message)
{
	using namespace re::literals;
	RE_LOG_INFO("C#"_logcat, "{}", message);
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

bool ScriptBinder::Input_IsKeyDown_Impl(int key)
{
	return Keyboard::IsKeyPressed(static_cast<Keyboard::Key>(key));
}

bool ScriptBinder::Input_IsMouseButtonDown_Impl(int button)
{
	return Mouse::IsButtonPressed(static_cast<Mouse::Button>(button));
}

} // namespace re::runtime