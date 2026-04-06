#include <Physics/Core.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <iostream>

namespace re::physics
{

bool Init()
{
	JPH::RegisterDefaultAllocator();

	JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

	std::cout << "[Physics] Jolt Physics initialized successfully.\n";

	return true;
}

void Shutdown()
{
	JPH::UnregisterTypes();

	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;

	std::cout << "[Physics] Physics subsystem shutdown.\n";
}

} // namespace re::physics