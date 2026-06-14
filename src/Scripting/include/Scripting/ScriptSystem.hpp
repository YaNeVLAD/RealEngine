#pragma once

#include <Core/String.hpp>
#include <ECS/System/System.hpp>
#include <RVM/VirtualMachine.hpp>
#include <Scripting/ScriptComponent.hpp>

namespace re
{

class ScriptSystem : public ecs::System
{
public:
	explicit ScriptSystem(rvm::VirtualMachine* vm)
		: m_vm(vm)
	{
	}

	void Update(ecs::Scene& scene, const core::TimeDelta deltaTime) override
	{
		m_vm->SetUserData(&scene);

		for (auto&& [entity, script] : *scene.CreateView<ScriptComponent>())
		{
			if (!script.isAwakeCalled)
			{
				script.instanceHandle = m_vm->Instantiate(script.className);

				if (!std::holds_alternative<rvm::Null_t>(script.instanceHandle))
				{
					m_vm->InvokeMethod(script.instanceHandle, "OnStart", { static_cast<rvm::Int>(entity) });
				}
				else
				{
					std::cerr << "[ECS] Failed to instantiate script: " << script.className << "\n";
				}

				script.isAwakeCalled = true;
			}

			if (script.isAwakeCalled && !std::holds_alternative<rvm::Null_t>(script.instanceHandle))
			{
				m_vm->InvokeMethod(script.instanceHandle, "OnUpdate", { static_cast<rvm::Double>(deltaTime) });
			}
		}

		m_vm->SetUserData(nullptr);
	}

private:
	rvm::VirtualMachine* m_vm;
};

} // namespace re