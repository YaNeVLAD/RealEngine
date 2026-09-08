#pragma once

#include <Core/Math/Vector3.hpp>

namespace re::scripting
{

struct EngineApiPointers
{
	void (*NativeLog)(const char* message);
	void (*Transform_GetPosition)(std::uint64_t entityID, Vector3f* outPosition);
	void (*Transform_SetPosition)(std::uint64_t entityID, const Vector3f* inPosition);

	bool (*Input_IsKeyDown)(int key) = nullptr;
	bool (*Input_IsMouseButtonDown)(int button) = nullptr;
};

} // namespace re::scripting
