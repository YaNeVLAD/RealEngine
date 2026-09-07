#pragma once

#include <Core/String.hpp>
#include <ECS/System/System.hpp>
#include <RVM/VirtualMachine.hpp>
#include <Runtime/Components.hpp>

namespace re
{

class ScriptSystem : public ecs::System
{
public:
	explicit ScriptSystem(rvm::VirtualMachine* vm)
	{
	}

	void Update(ecs::Scene& scene, const core::TimeDelta deltaTime) override
	{
	}

private:
};

} // namespace re
