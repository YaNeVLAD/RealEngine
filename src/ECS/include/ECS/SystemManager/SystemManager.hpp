#pragma once

#include <ECS/ComponentManager/ComponentManager.hpp>
#include <ECS/System/System.hpp>
#include <ECS/TypeIndex/TypeIndex.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace re::ecs
{
using SystemId = TypeIndexType;

class SystemManager final
{
public:
	class SystemConfiguration
	{
	public:
		SystemConfiguration(SystemManager& manager, SystemId id)
			: m_id(id)
			, m_manager(manager)
		{
		}

		template <typename... TComponents>
		SystemConfiguration& WithRead()
		{
			m_manager.AddReadDependencies<TComponents...>(m_id);

			return *this;
		}

		template <typename TComponent>
		SystemConfiguration& WithWrite()
		{
			m_manager.AddWriteDependency<TComponent>(m_id);

			return *this;
		}

		SystemConfiguration& RunOnMainThread()
		{
			m_manager.RunSystemOnMainThread(m_id);

			return *this;
		}

	private:
		SystemId m_id;
		SystemManager& m_manager;
	};

public:
	SystemManager();
	~SystemManager();

	template <typename TSystem, typename... TArgs>
	SystemConfiguration RegisterSystem(TArgs&&... args);

	template <typename TSystem>
	bool IsSystemRegistered() const;

	template <typename TSystem>
	TSystem& GetSystem();

	template <typename TSystem>
	TSystem const& GetSystem() const;

	void OnEntitySignatureChanged(Entity entity, Signature entitySignature, Scene* scene);

	void BuildExecutionGraph();

	void Execute(Scene& scene, float dt);

private:
	template <typename... TComponents>
	void AddReadDependencies(SystemId systemId);

	template <typename TComponent>
	void AddWriteDependency(SystemId systemId);

	void RunSystemOnMainThread(SystemId systemId);

	void WorkerLoop();

private:
	SystemId m_currentSystemId = Entity::INVALID_ID;

	std::unordered_map<SystemId, std::unique_ptr<System>> m_systems;
	std::unordered_map<SystemId, Signature> m_signatures;
	std::unordered_map<SystemId, Signature> m_readDependencies;
	std::unordered_map<SystemId, ComponentType> m_writeDependencies;

	std::vector<std::vector<SystemId>> m_executionStages;

	std::unordered_map<Entity, size_t> m_entityToIndexMap;

	std::vector<std::jthread> m_workerThreads;
	std::queue<std::function<void()>> m_taskQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_workerCondition;
	std::condition_variable m_mainCondition;
	std::atomic<size_t> m_tasksInProgress = 0;
	std::atomic<bool> m_stopThreads = false;

	std::unordered_set<SystemId> m_mainThreadSystems;
};

} // namespace re::ecs

#include <ECS/SystemManager/SystemManager.inl>