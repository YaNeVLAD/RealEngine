#include <ECS/EntityWrapper/EntityWrapper.hpp>

namespace re::ecs
{

template <typename TScene>
EntityWrapper<TScene>::EntityWrapper(TScene* scene, const Entity id, const Signature signature)
	: m_scene(scene)
	, m_id(id)
	, m_signature(signature)
{
}

template <typename TScene>
Entity EntityWrapper<TScene>::GetEntity() const
{
	return m_id;
}

template <typename TScene>
Signature EntityWrapper<TScene>::GetSignature() const
{
	return m_signature;
}

template <typename TScene>
template <typename TComponent>
EntityWrapper<TScene> EntityWrapper<TScene>::AddComponent(TComponent const& component)
{
	m_scene->template AddComponent<TComponent>(m_id, component);

	return *this;
}

template <typename TScene>
template <typename TComponent, typename... TArgs>
EntityWrapper<TScene> EntityWrapper<TScene>::AddComponent(TArgs&&... args)
{
	m_scene->template AddComponent<TComponent>(
		m_id,
		TComponent{ std::forward<TArgs>(args)... });

	return *this;
}

template <typename TScene>
template <typename TComponent>
TComponent& EntityWrapper<TScene>::GetComponent()
{
	return m_scene->template GetComponent<TComponent>(m_id);
}

template <typename TScene>
template <typename TComponent>
TComponent const& EntityWrapper<TScene>::GetComponent() const
{
	return m_scene->template GetComponent<TComponent>(m_id);
}

template <typename TScene>
template <typename TComponent>
bool EntityWrapper<TScene>::HasComponent() const
{
	return m_scene->template HasComponent<TComponent>(m_id);
}

template <typename TScene>
template <typename TComponent>
void EntityWrapper<TScene>::RemoveComponent()
{
	m_scene->template RemoveComponent<TComponent>(m_id);
}

template <typename TScene>
void EntityWrapper<TScene>::Destroy()
{
	m_scene->DestroyEntity(m_id);
}

template <typename TScene>
auto EntityWrapper<TScene>::operator<=>(EntityWrapper const& other) const
{
	return m_id <=> other.m_id;
}

template <typename TScene>
bool EntityWrapper<TScene>::operator==(EntityWrapper const& other) const
{
	return m_id == other.m_id;
}

template <typename TScene>
bool EntityWrapper<TScene>::operator==(Entity const& entity) const
{
	return m_id == entity;
}

} // namespace re::ecs