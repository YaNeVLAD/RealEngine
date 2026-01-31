#pragma once

#include <ECS/ComponentManager/ComponentManager.hpp>
#include <ECS/View/IView.h>
#include <ECS/View/Iterator/ViewIterator.h>

#include <vector>

namespace re::ecs
{

template <typename... TComponents>
class View final : public IView
{
public:
	using Iterator = ViewIterator<false, TComponents...>;
	using ConstIterator = ViewIterator<true, TComponents...>;

	View(ComponentManager& manager, const Signature signature)
		: m_manager(manager)
		, m_signature(signature)
	{
	}

	auto begin() { return Iterator(m_manager, m_entities.begin()); }
	auto end() { return Iterator(m_manager, m_entities.end()); }

	auto begin() const { return ConstIterator(m_manager, m_entities.cbegin()); }
	auto end() const { return ConstIterator(m_manager, m_entities.cend()); }

	void AddEntity(const Entity entity)
	{
		m_entities.push_back(entity);
	}

	void OnEntityDestroyed(const Entity entity) override
	{
		std::erase(m_entities, entity);
	}

	void OnEntitySignatureChanged(const Entity entity, const Signature entitySignature) override
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

private:
	ComponentManager& m_manager;
	Signature m_signature;
	std::vector<Entity> m_entities;
};

} // namespace re::ecs