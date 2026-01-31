#pragma once

#include <ECS/ComponentManager/ComponentManager.hpp>

#include <tuple>
#include <vector>

namespace re::ecs
{

template <bool IsConst, typename... TComponents>
class ViewIterator final
{
public:
	using EntityIterator = std::conditional_t<IsConst,
		std::vector<Entity>::const_iterator,
		std::vector<Entity>::iterator>;

	using iterator_category = std::forward_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = std::conditional_t<IsConst,
		std::tuple<Entity, const TComponents&...>,
		std::tuple<Entity, TComponents&...>>;

	ViewIterator(ComponentManager& manager, EntityIterator it)
		: m_manager(manager)
		, m_it(it)
	{
	}

	ViewIterator& operator++()
	{
		++m_it;
		return *this;
	}

	ViewIterator operator++(int)
	{
		ViewIterator tmp = *this;
		++(*this);
		return tmp;
	}

	value_type operator*() const
	{
		Entity entity = *m_it;

		if constexpr (IsConst)
		{
			return std::forward_as_tuple(entity, std::as_const(m_manager).GetComponent<TComponents>(entity)...);
		}
		else
		{
			return std::forward_as_tuple(entity, m_manager.GetComponent<TComponents>(entity)...);
		}
	}

	bool operator!=(const ViewIterator& other) const { return m_it != other.m_it; }
	bool operator==(const ViewIterator& other) const { return m_it == other.m_it; }

private:
	ComponentManager& m_manager;
	EntityIterator m_it;
};

} // namespace re::ecs