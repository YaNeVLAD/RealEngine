#pragma once

#include <ECS/ComponentArray/ComponentArray.hpp>
#include <ECS/TypeIndex/TypeIndex.hpp>

#include <cassert>
#include <memory>
#include <ranges>
#include <unordered_map>

namespace re::ecs
{
using ComponentType = TypeIndexType;

class ComponentManager final
{
public:
	template <typename TComponent>
	void RegisterComponent();

	template <typename TComponent>
	[[nodiscard]] bool IsComponentRegistered() const;

	template <typename TComponent>
	void AddComponent(Entity entity, TComponent const& component);

	template <typename TComponent>
	void RemoveComponent(Entity entity);

	template <typename TComponent>
	TComponent& GetComponent(Entity entity);

	template <typename TComponent>
	TComponent const& GetComponent(Entity entity) const;

	template <typename TComponent>
	[[nodiscard]] bool HasComponent(Entity entity) const;

	void OnEntityDestroyed(Entity entity);

private:
	std::unordered_map<ComponentType, std::shared_ptr<IComponentArray>> m_componentArrays;

	template <typename TComponent>
	std::shared_ptr<ComponentArray<TComponent>> GetComponentArray() const;
};

} // namespace re::ecs

#include <ECS/ComponentManager/ComponentManager.inl>