#include <Engine/EngineContext.hpp>

uint64_t ReEngine_SceneCreateEntity(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_CreateEntity();
}

int32_t ReEngine_SceneIsEntityValid(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_IsEntityValid(entity) ? 1 : 0;
}

void ReEngine_SceneDestroyEntity(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_DestroyEntity(entity);
}

uint32_t ReEngine_SceneGetEntityCount(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_GetEntityCount();
}

int32_t ReEngine_SceneGetEntities(uint64_t* outIds, const uint32_t capacity, uint32_t* outTotal)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outIds || !outTotal)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Scene_GetEntities(outIds, capacity, outTotal) ? RE_ENGINE_OK : RE_ENGINE_FAILED;
}

uint32_t ReEngine_SceneClearEntities(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_ClearEntities();
}

uint64_t ReEngine_SceneSpawnPrimitive(const int32_t kind, const uint32_t rgba8888)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_SpawnPrimitive(kind, rgba8888);
}

void ReEngine_SceneConfirmChanges(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_ConfirmChanges();
}