#include <Physics/Core.hpp>

#include <Core/Logger.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

using namespace re::literals;

namespace re::physics
{

bool Init()
{
	JPH::RegisterDefaultAllocator();

	JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

	RE_LOG_INFO("Physics"_logcat, "Jolt Physics initialized successfully");

	return true;
}

void Shutdown()
{
	JPH::UnregisterTypes();

	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;

	RE_LOG_INFO("Physics"_logcat, "Jolt Physics subsystem shutdown");
}

} // namespace re::physics