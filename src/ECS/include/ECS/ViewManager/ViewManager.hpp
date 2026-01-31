#pragma once

#include <ECS/EntityManager/EntityManager.hpp>
#include <ECS/View/View.hpp>

#include <memory>

namespace re::ecs
{

class ViewManager final
{
public:
	template <typename... TComponents>
	std::shared_ptr<View<TComponents...>>
	CreateView(ComponentManager& componentManager, EntityManager const& entityManager);

	void OnEntityDestroyed(Entity entity);

	void OnEntitySignatureChanged(Entity entity, Signature signature);

private:
	template <typename... TComponents>
	Signature CreateSignature();

	std::unordered_map<Signature, std::shared_ptr<IView>> m_views;
};

} // namespace re::ecs

#include <ECS/ViewManager/ViewManager.inl>