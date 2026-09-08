#pragma once

#include <ECS/System/System.hpp>
#include <Runtime/Internal/ScriptBinder.hpp>
#include <Scripting/Interface/IScriptEngine.hpp>

namespace re
{

class ScriptSystem : public ecs::System
{
public:
	explicit ScriptSystem(scripting::IScriptEngine* scriptEngine)
		: m_scriptEngine(scriptEngine)
	{
	}

	void Update(ecs::Scene& scene, const core::TimeDelta deltaTime) override
	{
		if (!m_scriptEngine)
		{
			return;
		}

		runtime::ScriptBinder::SetActiveScene(&scene);

		for (auto&& [entity, script] : *scene.CreateView<ScriptComponent>())
		{
			if (!script.Instance)
			{
				if (!script.ScriptClass)
				{
					script.ScriptClass = m_scriptEngine->GetClass(script.Namespace, script.Class);
				}

				if (script.ScriptClass)
				{
					script.Instance = script.ScriptClass->Instantiate(entity);
					if (script.Instance)
					{
						script.Instance->OnCreate();
					}
				}
			}

			if (script.Instance)
			{
				script.Instance->OnUpdate(deltaTime);
			}
		}

		runtime::ScriptBinder::SetActiveScene(nullptr);
	}

private:
	scripting::IScriptEngine* m_scriptEngine = nullptr;
};

} // namespace re
