#pragma once

#include <Core/Math/Vector3.hpp>

#include <cstdint>

#include "bridge/ReflectionIds.gen.hpp"

namespace re::scripting
{

inline constexpr std::uint32_t EngineApiVersion = 4;

struct ComponentFieldInfo
{
	char name[48];
	FieldType type;
	std::uint32_t size;
};

struct EngineApiPointers
{
	std::uint32_t apiVersion;
	void (*NativeLog)(const char* message);
	void (*OnScriptError)(const char* message);

	std::uint64_t (*Scene_CreateEntity)();
	bool (*Scene_IsEntityValid)(std::uint64_t entityID);
	void (*Scene_DestroyEntity)(std::uint64_t entityID);

	bool (*Input_IsKeyDown)(int key);
	bool (*Input_IsMouseButtonDown)(int button);

	std::uint64_t (*Entity_GetComponentMask)(std::uint64_t entityID);
	bool (*Entity_AddComponent)(std::uint64_t entityID, ReflectedComponentId component);
	bool (*Entity_RemoveComponent)(std::uint64_t entityID, ReflectedComponentId component);
	bool (*Entity_GetFieldData)(std::uint64_t entityID, ReflectedComponentId component, std::int32_t fieldIndex, void* outData, std::uint32_t maxBytes, std::uint32_t* outBytes);
	bool (*Entity_SetFieldData)(std::uint64_t entityID, ReflectedComponentId component, std::int32_t fieldIndex, const void* data, std::uint32_t bytes);

	std::int32_t (*Component_GetFieldCount)(ReflectedComponentId component);
	bool (*Component_GetFieldInfo)(ReflectedComponentId component, std::int32_t fieldIndex, ComponentFieldInfo* outInfo);
};

} // namespace re::scripting