#include <ECS/View/View.hpp>

namespace re::ecs
{

template <typename... TComponents>
View<TComponents...>::View(ComponentManager& manager, const Signature signature)
	: m_manager(manager)
	, m_signature(signature)
{
}

template <typename... TComponents>
auto View<TComponents...>::begin()
{
	return Iterator(m_manager, m_entities.begin());
}

template <typename... TComponents>
auto View<TComponents...>::end()
{
	return Iterator(m_manager, m_entities.end());
}

template <typename... TComponents>
auto View<TComponents...>::begin() const
{
	return ConstIterator(m_manager, m_entities.cbegin());
}

template <typename... TComponents>
auto View<TComponents...>::end() const
{
	return ConstIterator(m_manager, m_entities.cend());
}

template <typename... TComponents>
void View<TComponents...>::AddEntity(const Entity entity)
{
	m_entities.push_back(entity);
}

template <typename... TComponents>
void View<TComponents...>::OnEntityDestroyed(const Entity entity)
{
	std::erase(m_entities, entity);
}

template <typename... TComponents>
void View<TComponents...>::OnEntitySignatureChanged(const Entity entity, const Signature entitySignature)
{
	if ((entitySignature & m_signature) == m_signature)
	{
		if (std::ranges::find(m_entities, entity) == m_entities.end())
		{
			m_entities.push_back(entity);
		}
	}
	else
	{
		std::erase(m_entities, entity);
	}
}

} // namespace re::ecs