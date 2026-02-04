#include <ECS/ViewManager/ViewManager.hpp>

namespace re::ecs
{

template <typename... TComponents>
std::shared_ptr<View<TComponents...>> ViewManager::CreateView(
	ComponentManager& componentManager, EntityManager const& entityManager)
{
	Signature signature = CreateSignature<TComponents...>();

	if (m_views.contains(signature))
	{
		return std::dynamic_pointer_cast<View<TComponents...>>(m_views.at(signature));
	}

	auto view = std::make_shared<View<TComponents...>>(componentManager, signature);

	for (Entity entity : entityManager.GetActiveEntities())
	{
		if ((entityManager.GetSignature(entity) & signature) == signature)
		{
			view->AddEntity(entity);
		}
	}

	m_views[signature] = view;
	return view;
}

template <typename... TComponents>
Signature ViewManager::CreateSignature()
{
	Signature signature;
	(signature.set(TypeIndex<TComponents>()), ...);
	return signature;
}

} // namespace re::ecs