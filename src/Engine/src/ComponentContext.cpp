#include <Engine/EngineContext.hpp>

#include <cstddef>

static_assert(sizeof(ReEngine_ComponentFieldInfo) == sizeof(re::scripting::ComponentFieldInfo));
static_assert(offsetof(ReEngine_ComponentFieldInfo, name) == offsetof(re::scripting::ComponentFieldInfo, name));
static_assert(offsetof(ReEngine_ComponentFieldInfo, type) == offsetof(re::scripting::ComponentFieldInfo, type));
static_assert(offsetof(ReEngine_ComponentFieldInfo, size) == offsetof(re::scripting::ComponentFieldInfo, size));

uint64_t ReEngine_EntityGetComponentMask(uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Entity_GetComponentMask(entity);
}

int32_t ReEngine_EntityAddComponent(const uint64_t entity, int32_t component)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	return host.scene.api.Entity_AddComponent(entity, static_cast<re::scripting::ReflectedComponentId>(component))
		? RE_ENGINE_OK
		: RE_ENGINE_FAILED;
}

int32_t ReEngine_EntityRemoveComponent(const uint64_t entity, int32_t component)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	return host.scene.api.Entity_RemoveComponent(entity, static_cast<re::scripting::ReflectedComponentId>(component))
		? RE_ENGINE_OK
		: RE_ENGINE_FAILED;
}

int32_t ReEngine_EntityGetFieldData(
	const uint64_t entity,
	int32_t component,
	const int32_t fieldIndex,
	void* outData,
	const uint32_t maxBytes,
	uint32_t* outBytes)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outData || !outBytes)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Entity_GetFieldData(
			   entity, static_cast<re::scripting::ReflectedComponentId>(component), fieldIndex, outData, maxBytes, outBytes)
		? RE_ENGINE_OK
		: RE_ENGINE_FAILED;
}

int32_t ReEngine_EntitySetFieldData(
	const uint64_t entity,
	int32_t component,
	const int32_t fieldIndex,
	const void* data,
	const uint32_t bytes)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!data)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Entity_SetFieldData(
			   entity, static_cast<re::scripting::ReflectedComponentId>(component), fieldIndex, data, bytes)
		? RE_ENGINE_OK
		: RE_ENGINE_FAILED;
}

int32_t ReEngine_EntityGetName(const uint64_t entity, char* outName, const uint32_t capacity, uint32_t* outNeeded)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outNeeded)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}
	if (!host.scene.api.Scene_IsEntityValid(entity))
	{
		return RE_ENGINE_INVALID_HANDLE;
	}

	if (host.scene.api.Entity_GetName(entity, outName, capacity, outNeeded))
	{
		return RE_ENGINE_OK;
	}

	*outNeeded = 0;
	return RE_ENGINE_OK;
}

int32_t ReEngine_EntitySetName(const uint64_t entity, const char* nameUtf8)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!nameUtf8)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Entity_SetName(entity, nameUtf8) ? RE_ENGINE_OK : RE_ENGINE_FAILED;
}

int32_t ReEngine_ComponentGetFieldCount(int32_t component)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return -1;
	}

	return host.scene.api.Component_GetFieldCount(static_cast<re::scripting::ReflectedComponentId>(component));
}

int32_t ReEngine_ComponentGetFieldInfo(int32_t component, const int32_t fieldIndex, ReEngine_ComponentFieldInfo* outInfo)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outInfo)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Component_GetFieldInfo(
			   static_cast<re::scripting::ReflectedComponentId>(component),
			   fieldIndex,
			   reinterpret_cast<re::scripting::ComponentFieldInfo*>(outInfo))
		? RE_ENGINE_OK
		: RE_ENGINE_FAILED;
}