#pragma once

#include <ECS/System/System.hpp>
#include <Runtime/Internal/ScriptBinder.hpp>
#include <Scripting/Interface/IScriptEngine.hpp>

#include <unordered_map>

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
						m_instances[entity.Id()] = script.Instance;
						script.Instance->OnCreate();
					}
				}
			}

			if (script.Instance)
			{
				script.Instance->OnUpdate(deltaTime);
			}
		}

		ReleaseDestroyedInstances(scene);

		runtime::ScriptBinder::SetActiveScene(nullptr);
	}

private:
	void ReleaseDestroyedInstances(const ecs::Scene& scene)
	{
		for (auto it = m_instances.begin(); it != m_instances.end();)
		{
			const auto entity = ecs::Entity{ it->first };
			const bool stillAlive = scene.IsValid(entity) && scene.HasComponent<ScriptComponent>(entity);

			if (stillAlive)
			{
				++it;
				continue;
			}

			it->second->Release();
			it = m_instances.erase(it);
		}
	}

private:
	scripting::IScriptEngine* m_scriptEngine = nullptr;
	std::unordered_map<std::uint64_t, std::shared_ptr<scripting::IScriptInstance>> m_instances;
};

} // namespace re