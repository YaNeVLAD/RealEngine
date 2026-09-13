#pragma once

#include <Core/Math/Vector3.hpp>

#include <cstdint>

namespace re::scripting
{

inline constexpr std::uint32_t EngineApiVersion = 2;

enum class ComponentDataKind : std::int32_t
{
	Transform = 0,
	Camera = 1,
	Light = 2,
};

struct TransformComponentData
{
	float position[3]; // X, Y, Z
	float rotation[3]; // euler, degrees
	float scale[3];
};

struct CameraComponentData
{
	float fov; // degrees
	float nearClip;
	float farClip;
	float zoom;
	std::uint32_t isPrimal;
};

struct LightComponentData
{
	std::int32_t type; // 0 = Directional, 1 = Light (Point), 2 = Spot
	float color[3]; // RGB diffuse, normalized
	float falloff;
	float cutOffAngle;
	float ambientIntensity;
};

struct EngineApiPointers
{
	std::uint32_t apiVersion;
	void (*NativeLog)(const char* message);
	void (*OnScriptError)(const char* message);

	std::uint64_t (*Scene_CreateEntity)();
	bool (*Scene_IsEntityValid)(std::uint64_t entityID);
	void (*Scene_DestroyEntity)(std::uint64_t entityID);

	bool (*Entity_GetComponentData)(std::uint64_t entityID, ComponentDataKind kind, void* outData, std::uint32_t maxBytes, std::uint32_t* outBytes);
	bool (*Entity_SetComponentData)(std::uint64_t entityID, ComponentDataKind kind, const void* data, std::uint32_t bytes);

	bool (*Input_IsKeyDown)(int key);
	bool (*Input_IsMouseButtonDown)(int button);
};

} // namespace re::scripting