#pragma once

#include <ECS/Entity/Entity.hpp>
#include <ECS/Entity/Signature.hpp>

namespace re::ecs
{

template <typename TScene>
class EntityWrapper final
{
public:
	EntityWrapper(TScene* scene, Entity id, Signature signature)
		: m_scene(scene)
		, m_id(id)
		, m_signature(signature)
	{
	}

	[[nodiscard]] Entity GetEntity() const
	{
		return m_id;
	}

	[[nodiscard]] Signature GetSignature() const
	{
		return m_signature;
	}

	template <typename TComponent>
	void AddComponent(TComponent const& component)
	{
		m_scene->template AddComponent<TComponent>(m_id, component);
	}

	template <typename TComponent, typename... TArgs>
	void AddComponent(TArgs&&... args)
	{
		m_scene->template AddComponent<TComponent>(
			m_id,
			TComponent{ std::forward<TArgs>(args)... });
	}

	template <typename TComponent>
	TComponent& GetComponent()
	{
		return m_scene->template GetComponent<TComponent>(m_id);
	}

	template <typename TComponent>
	TComponent const& GetComponent() const
	{
		return m_scene->template GetComponent<TComponent>(m_id);
	}

	template <typename TComponent>
	[[nodiscard]] bool HasComponent() const
	{
		return m_scene->template HasComponent<TComponent>(m_id);
	}

	template <typename TComponent>
	void RemoveComponent()
	{
		m_scene->template RemoveComponent<TComponent>(m_id);
	}

	void Destroy()
	{
		m_scene->DestroyEntity(m_id);
	}

	auto operator<=>(EntityWrapper const& other) const
	{
		return m_id <=> other.m_id;
	}

	bool operator==(EntityWrapper const& other) const
	{
		return m_id == other.m_id;
	}

	bool operator==(Entity const& entity) const
	{
		return m_id == entity;
	}

private:
	TScene* m_scene;
	Entity m_id;
	Signature m_signature;
};

} // namespace re::ecs