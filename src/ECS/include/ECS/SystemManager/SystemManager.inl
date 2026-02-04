#include <ECS/SystemManager/SystemManager.hpp>

namespace re::ecs
{

template <typename... TComponents>
SystemManager::SystemConfiguration& SystemManager::SystemConfiguration::WithRead()
{
	m_manager.AddReadDependencies<TComponents...>(m_id);

	return *this;
}

template <typename TComponent>
SystemManager::SystemConfiguration& SystemManager::SystemConfiguration::WithWrite()
{
	m_manager.AddWriteDependency<TComponent>(m_id);

	return *this;
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
void SystemManager::AddWriteDependency(const SystemId systemId)
{
	assert(!m_writeDependencies.contains(systemId) && "System can only have one write dependency.");

	const ComponentType componentType = TypeIndex<TComponent>();
	m_signatures[systemId].set(componentType);
	m_writeDependencies[systemId] = componentType;
}

template <typename TSystem>
TSystem& SystemManager::GetSystem()
{
	const SystemId systemId = TypeIndex<TSystem>();
	assert(m_systems.contains(systemId)
		&& "Cannot get system: not registered.");

	return *static_cast<TSystem*>(m_systems.at(systemId).get());
}

template <typename TSystem>
TSystem const& SystemManager::GetSystem() const
{
	return const_cast<SystemManager const&>(*this).GetSystem<TSystem>();
}

} // namespace re::ecs