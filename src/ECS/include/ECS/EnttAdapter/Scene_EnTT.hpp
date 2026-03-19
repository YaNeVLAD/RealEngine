#pragma once

#include <ECS/Entity/Entity.hpp>
#include <ECS/EntityWrapper/EntityWrapper.hpp>
#include <ECS/SystemManager/SystemManager.hpp>

#include <entt/entt.hpp>
#include <memory>
#include <tuple>
#include <vector>

namespace re::ecs
{

template <typename... TComponents>
class EnttViewAdapter
{
public:
	explicit EnttViewAdapter(entt::registry& registry)
		: m_view(registry.view<TComponents...>())
	{
	}

	struct Iterator
	{
		using BaseIterator = decltype(std::declval<entt::view<entt::get_t<TComponents...>>>().each().begin());
		BaseIterator it;

		auto operator*()
		{
			return std::apply([]<typename... T>(entt::entity e, T&... comps) {
				return std::tuple<Entity, T&...>(Entity{ static_cast<uint32_t>(e) }, comps...);
			},
				*it);
		}

		Iterator& operator++()
		{
			++it;
			return *this;
		}
		bool operator!=(const Iterator& other) const { return it != other.it; }
	};

	Iterator begin() { return { m_view.each().begin() }; }
	Iterator end() { return { m_view.each().end() }; }

private:
	entt::view<entt::get_t<TComponents...>> m_view;
};

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
		m_entitiesToDestroy.push_back(entity);
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
	std::shared_ptr<EnttViewAdapter<TComponents...>> CreateView()
	{
		return std::make_shared<EnttViewAdapter<TComponents...>>(m_registry);
	}

private:
	entt::registry m_registry;
	std::unique_ptr<SystemManager> m_systemManager;

	std::vector<Entity> m_entitiesToDestroy;
};

} // namespace re::ecs