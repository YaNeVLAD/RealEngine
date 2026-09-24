#include <Engine/EngineContext.hpp>

uint64_t ReEngine_Scene_CreateEntity(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_CreateEntity();
}

int32_t ReEngine_Scene_IsEntityValid(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_IsEntityValid(entity) ? 1 : 0;
}

void ReEngine_Scene_DestroyEntity(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_DestroyEntity(entity);
}

uint32_t ReEngine_Scene_GetEntityCount(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_GetEntityCount();
}

int32_t ReEngine_Scene_GetEntities(uint64_t* outIds, const uint32_t capacity, uint32_t* outTotal)
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

uint32_t ReEngine_Scene_ClearEntities(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_ClearEntities();
}

uint64_t ReEngine_Scene_SpawnPrimitive(const int32_t kind, const uint32_t rgba8888)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_SpawnPrimitive(kind, rgba8888);
}

void ReEngine_Scene_ConfirmChanges(void)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_ConfirmChanges();
}