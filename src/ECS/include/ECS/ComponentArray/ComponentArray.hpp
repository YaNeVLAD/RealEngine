#pragma once

#include <ECS/ComponentArray/IComponentArray.hpp>

#include <vector>

namespace re::ecs
{

template <typename TComponent>
class ComponentArray final : public IComponentArray
{
public:
	void AddComponent(Entity entity, TComponent const& component);

	void RemoveComponent(Entity entity);

	TComponent& GetComponent(Entity entity);

	TComponent const& GetComponent(Entity entity) const;

	[[nodiscard]] bool HasComponent(Entity entity) const;

	std::vector<TComponent>& GetComponents();

	void OnEntityDestroyed(Entity entity) override;

private:
	static constexpr size_t InvalidIndex = std::numeric_limits<size_t>::max();

	std::vector<TComponent> m_components;

	std::vector<size_t> m_sparse;

	std::vector<Entity> m_denseToEntity;
};

} // namespace re::ecs

#include <ECS/ComponentArray/ComponentArray.inl>
