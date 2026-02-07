#pragma once

#include <ECS/ComponentManager/ComponentManager.hpp>
#include <ECS/Entity/Entity.hpp>
#include <ECS/EntityManager/EntityManager.hpp>
#include <ECS/EntityWrapper/EntityWrapper.hpp>
#include <ECS/SystemManager/SystemManager.hpp>
#include <ECS/ViewManager/ViewManager.hpp>

#include <memory>
#include <vector>

namespace re::ecs
{

class Scene final
{
public:
	Scene()
		: m_componentManager(std::make_unique<ComponentManager>())
		, m_entityManager(std::make_unique<EntityManager>())
		, m_systemManager(std::make_unique<SystemManager>())
		, m_viewManager(std::make_unique<ViewManager>())
	{
	}

	EntityWrapper<Scene> CreateEntity()
	{
		const auto entity = m_entityManager->CreateEntity();
		const auto signature = m_entityManager->GetSignature(entity);

		return { this, entity, signature };
	}

	void DestroyEntity(const Entity entity)
	{
		m_entitiesToDestroy.push_back(entity);
	}

	template <typename TComponent>
	void RegisterComponent()
	{
		m_componentManager->RegisterComponent<TComponent>();
	}

	template <typename... TComponents>
	void RegisterComponents()
	{
		(m_componentManager->RegisterComponent<TComponents>(), ...);
	}

	template <typename T>
	bool IsRegistered() const
	{
		if constexpr (std::is_base_of_v<System, T>)
		{
			return m_systemManager->IsSystemRegistered<T>();
		}

		return m_componentManager->IsComponentRegistered<T>();
	}

	bool IsValid(const Entity entity) const
	{
		return m_entityManager->IsValid(entity);
	}

	template <typename TComponent>
	void AddComponent(Entity entity, TComponent const& component)
	{
		AddComponentImpl(entity, component);
	}

	template <typename TComponent, typename... TArgs>
	void AddComponent(Entity entity, TArgs&&... args)
	{
		auto component = TComponent{ std::forward<TArgs>(args)... };
		AddComponentImpl(entity, std::move(component));
	}

	template <typename TComponent>
	void RemoveComponent(const Entity entity)
	{
		m_componentManager->RemoveComponent<TComponent>(entity);

		auto& signature = m_entityManager->GetSignature(entity);
		signature.set(TypeIndex<TComponent>());
		m_entityManager->SetSignature(entity, signature);

		m_systemManager->OnEntitySignatureChanged(entity, signature, this);
	}

	template <typename TComponent>
	TComponent& GetComponent(const Entity entity)
	{
		return m_componentManager->GetComponent<TComponent>(entity);
	}

	template <typename TComponent>
	TComponent const& GetComponent(const Entity entity) const
	{
		return m_componentManager->GetComponent<TComponent>(entity);
	}

	template <typename TComponent>
	bool HasComponent(const Entity entity) const
	{
		return m_componentManager->HasComponent<TComponent>(entity);
	}

	template <typename TSystem, typename... TArgs>
	SystemManager::SystemConfiguration AddSystem(TArgs&&... args)
	{
		return m_systemManager->RegisterSystem<TSystem>(std::forward<TArgs>(args)...);
	}

	template <typename TSystem>
	TSystem& GetSystem() const
	{
		return m_systemManager->GetSystem<TSystem>();
	}

	void BuildSystemGraph()
	{
		m_systemManager->BuildExecutionGraph();
	}

	void Frame(const float dt)
	{
		m_systemManager->Execute(*this, dt);
	}

	void ConfirmChanges()
	{
		for (Entity const& entity : m_entitiesToDestroy)
		{
			m_entityManager->DestroyEntity(entity);
			m_componentManager->OnEntityDestroyed(entity);
			m_systemManager->OnEntitySignatureChanged(entity, {}, this);
			m_viewManager->OnEntityDestroyed(entity);
		}

		m_entitiesToDestroy.clear();
	}

	template <typename... TComponents>
	std::shared_ptr<View<TComponents...>> CreateView()
	{
		return m_viewManager->CreateView<TComponents...>(*m_componentManager, *m_entityManager);
	}

private:
	template <typename TComponent>
	void AddComponentImpl(Entity entity, TComponent component)
	{
		if (!m_componentManager->IsComponentRegistered<TComponent>())
		{
			m_componentManager->RegisterComponent<TComponent>();
		}

		m_componentManager->AddComponent(entity, component);

		auto& signature = m_entityManager->GetSignature(entity);
		signature.set(TypeIndex<TComponent>());
		m_entityManager->SetSignature(entity, signature);

		m_systemManager->OnEntitySignatureChanged(entity, signature, this);
		m_viewManager->OnEntitySignatureChanged(entity, signature);
	}

private:
	std::unique_ptr<ComponentManager> m_componentManager;
	std::unique_ptr<EntityManager> m_entityManager;
	std::unique_ptr<SystemManager> m_systemManager;
	std::unique_ptr<ViewManager> m_viewManager;

	std::vector<Entity> m_entitiesToDestroy;
};

} // namespace re::ecs
