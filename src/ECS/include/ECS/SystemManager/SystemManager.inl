#include <ECS/SystemManager/SystemManager.hpp>

namespace re::ecs
{

inline SystemManager::SystemManager()
{
	const size_t threadCount = std::thread::hardware_concurrency();
	m_workerThreads.reserve(threadCount);
	for (size_t i = 0; i < threadCount; ++i)
	{
		m_workerThreads.emplace_back(&SystemManager::WorkerLoop, this);
	}
}

inline SystemManager::~SystemManager()
{
	m_stopThreads = true;
	m_workerCondition.notify_all();
}

template <typename TSystem, typename... TArgs>
SystemManager::SystemConfiguration SystemManager::RegisterSystem(TArgs&&... args)
{
	const SystemId systemId = TypeIndex<TSystem>();

	assert(!m_systems.contains(systemId)
		&& "Registering system more than once.");

	m_systems[systemId] = std::make_unique<TSystem>(std::forward<TArgs>(args)...);
	m_signatures[systemId] = Signature{};
	m_readDependencies[systemId] = Signature{};

	return { *this, systemId };
}

template <typename TSystem>
bool SystemManager::IsSystemRegistered() const
{
	return m_systems.contains(TypeIndex<TSystem>());
}

template <typename... TComponents>
void SystemManager::AddReadDependencies(const SystemId systemId)
{
	([&] {
		const ComponentType componentType = TypeIndex<TComponents>();
		m_signatures[systemId].set(componentType);
		m_readDependencies[systemId].set(componentType);
	}(),
		...);
}

template <typename TComponent>
void SystemManager::AddWriteDependency(SystemId systemId)
{
	assert(!m_writeDependencies.contains(systemId) && "System can only have one write dependency.");

	const ComponentType componentType = TypeIndex<TComponent>();
	m_signatures[systemId].set(componentType);
	m_writeDependencies[systemId] = componentType;
}

template <typename TSystem>
TSystem& SystemManager::GetSystem()
{
	SystemId systemId = TypeIndex<TSystem>();
	assert(m_systems.contains(systemId)
		&& "Cannot get system: not registered.");

	return *static_cast<TSystem*>(m_systems.at(systemId).get());
}

template <typename TSystem>
TSystem const& SystemManager::GetSystem() const
{
	return const_cast<SystemManager const&>(*this).GetSystem<TSystem>();
}

inline void SystemManager::OnEntitySignatureChanged(Entity entity, Signature entitySignature, Scene* scene)
{
	for (auto const& [id, system] : m_systems)
	{
		const Signature& systemSignature = m_signatures.at(id);
		const bool hasEntity = system->EntityToIndexMap.contains(entity);
		const bool signatureMatch = (entitySignature & systemSignature) == systemSignature;

		if (signatureMatch && !hasEntity)
		{
			system->EntityToIndexMap[entity] = system->Entities.size();
			system->Entities.emplace_back(scene, entity, entitySignature);
		}
		else if (!signatureMatch && hasEntity)
		{
			if (system->Entities.size() == 1)
			{
				system->Entities.clear();
				system->EntityToIndexMap.clear();
				continue;
			}

			const size_t indexOfRemoved = system->EntityToIndexMap.at(entity);
			Entity lastEntity = system->Entities.back().GetEntity();

			system->Entities[indexOfRemoved] = system->Entities.back();
			system->EntityToIndexMap[lastEntity] = indexOfRemoved;

			system->Entities.pop_back();
			system->EntityToIndexMap.erase(entity);
		}
	}
}

inline void SystemManager::BuildExecutionGraph()
{
	std::unordered_map<SystemId, int> inDegree;
	std::unordered_map<SystemId, std::vector<SystemId>> adjList;

	for (auto const& [writerId, writtenComponent] : m_writeDependencies)
	{
		for (auto const& [readerId, readSignature] : m_readDependencies)
		{
			if (readerId != writerId && readSignature.test(writtenComponent))
			{
				adjList[writerId].push_back(readerId);
				inDegree[readerId]++;
			}
		}
	}

	std::queue<SystemId> q;
	for (const auto& id : m_systems | std::views::keys)
	{
		if (!inDegree.contains(id))
		{
			q.push(id);
		}
	}

	m_executionStages.clear();
	while (!q.empty())
	{
		size_t stageSize = q.size();
		std::vector<SystemId> currentStage;
		for (size_t i = 0; i < stageSize; ++i)
		{
			SystemId u = q.front();
			q.pop();
			currentStage.push_back(u);

			if (adjList.contains(u))
			{
				for (SystemId v : adjList.at(u))
				{
					inDegree[v]--;
					if (inDegree[v] == 0)
					{
						q.push(v);
					}
				}
			}
		}
		m_executionStages.push_back(currentStage);
	}

	size_t scheduledSystems = 0;
	for (const auto& stage : m_executionStages)
	{
		scheduledSystems += stage.size();
	}

	assert(scheduledSystems == m_systems.size() && "Cycle detected in system dependencies!");
}

inline void SystemManager::Execute(Scene& scene, float dt)
{
	for (const auto& stage : m_executionStages)
	{
		m_tasksInProgress = stage.size();

		for (SystemId id : stage)
		{
			{
				std::unique_lock lock(m_queueMutex);
				m_taskQueue.emplace([this, id, &scene, dt]() {
					m_systems.at(id)->Update(scene, dt);
				});
			}
		}

		m_workerCondition.notify_all();

		{
			std::unique_lock lock(m_queueMutex);
			m_mainCondition.wait(lock, [this]() {
				return m_tasksInProgress == 0;
			});
		}
	}
}

inline void SystemManager::WorkerLoop()
{
	while (!m_stopThreads)
	{
		std::function<void()> task;
		{
			std::unique_lock lock(m_queueMutex);
			m_workerCondition.wait(lock, [this] {
				return m_stopThreads || !m_taskQueue.empty();
			});

			if (m_stopThreads && m_taskQueue.empty())
			{
				return;
			}

			task = std::move(m_taskQueue.front());
			m_taskQueue.pop();
		}

		task();

		if (--m_tasksInProgress == 0)
		{
			std::lock_guard lock(m_queueMutex);
			m_mainCondition.notify_one();
		}
	}
}

} // namespace re::ecs