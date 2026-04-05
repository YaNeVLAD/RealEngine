#pragma once

#include <ECS/Components.hpp>
#include <ECS/Entity/Entity.hpp>
#include <ECS/EntityWrapper/EntityWrapper.hpp>
#include <ECS/EnttAdapter/EnttViewAdapter.hpp>
#include <ECS/SystemManager/SystemManager.hpp>

#include <entt/entt.hpp>
#include <memory>
#include <vector>

namespace re::ecs
{

class Scene final
{
public:
	Scene()
		: m_systemManager(std::make_unique<SystemManager>())
	{
	}

	EntityWrapper<Scene> CreateEntity()
	{
		const auto entityId = static_cast<uint32_t>(m_registry.create());
		const Entity entity{ entityId };

		return { this, entity, {} };
	}

	void DestroyEntity(const Entity entity)
	{
		m_entitiesToDestroy.emplace(entity);
	}

	template <typename>
	void RegisterComponent()
	{
	}

	template <typename...>
	void RegisterComponents()
	{
	}

	template <typename T>
	bool IsRegistered() const
	{
		if constexpr (std::is_base_of_v<System, T>)
		{
			return m_systemManager->IsSystemRegistered<T>();
		}
		return true;
	}

	bool IsValid(const Entity entity) const
	{
		return m_registry.valid(static_cast<entt::entity>(entity.Id()));
	}

	template <typename TComponent>
	void AddComponent(const Entity entity, TComponent const& component)
	{
		m_registry.emplace_or_replace<TComponent>(static_cast<entt::entity>(entity.Id()), component);
	}

	template <typename TComponent, typename... TArgs>
	void AddComponent(const Entity entity, TArgs&&... args)
	{
		m_registry.emplace_or_replace<TComponent>(static_cast<entt::entity>(entity.Id()), std::forward<TArgs>(args)...);
	}

	template <typename TComponent>
	void RemoveComponent(const Entity entity)
	{
		m_registry.remove<TComponent>(static_cast<entt::entity>(entity.Id()));
	}

	template <typename TComponent>
	TComponent& GetComponent(const Entity entity)
	{
		return m_registry.get<TComponent>(static_cast<entt::entity>(entity.Id()));
	}

	template <typename TComponent>
	TComponent const& GetComponent(const Entity entity) const
	{
		return m_registry.get<TComponent>(static_cast<entt::entity>(entity.Id()));
	}

	template <typename TComponent>
	bool HasComponent(const Entity entity) const
	{
		return m_registry.all_of<TComponent>(static_cast<entt::entity>(entity.Id()));
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
			m_registry.destroy(static_cast<entt::entity>(entity.Id()));
		}

		m_entitiesToDestroy.clear();
	}

	template <typename... TComponents>
	void MakeDirty(const Entity entity)
	{
		(AddComponent<detail::DirtyTag<TComponents>>(entity), ...);
	}

	template <typename... TComponents>
	std::shared_ptr<EnttViewAdapter<TComponents...>> CreateView()
	{
		return std::make_shared<EnttViewAdapter<TComponents...>>(m_registry);
	}

	template <typename... TComponents>
	EntityWrapper<Scene> FindFirstWith()
	{
		if (auto view = m_registry.view<TComponents...>(); view.begin() != view.end())
		{
			entt::entity e = *view.begin();
			return { this, Entity{ static_cast<uint32_t>(e) }, {} };
		}

		// Возвращаем невалидную сущность (INVALID_ID)
		return { this, Entity{ Entity::INVALID_ID }, {} };
	}

	template <typename TComponent>
	TComponent* FindComponent()
	{
		if (const auto view = m_registry.view<TComponent>(); view.begin() != view.end())
		{
			return &view.template get<TComponent>(*view.begin());
		}

		return nullptr;
	}

	template <typename... TComponents>
	std::vector<Entity> FindEntitiesWith()
	{
		std::vector<Entity> result;
		auto view = m_registry.view<TComponents...>();

		result.reserve(std::distance(view.begin(), view.end()));

		for (auto entity : view)
		{
			result.emplace_back(static_cast<std::uint32_t>(entity));
		}

		return result;
	}

	template <typename... TComponents>
	bool ContainsAny()
	{
		auto view = m_registry.view<TComponents...>();

		return view.begin() != view.end();
	}

	entt::registry& GetRegistry()
	{
		return m_registry;
	}

private:
	entt::registry m_registry;
	std::unique_ptr<SystemManager> m_systemManager;

	std::unordered_set<Entity> m_entitiesToDestroy;
};

} // namespace re::ecs
