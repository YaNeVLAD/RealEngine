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

	ViewIterator(ComponentManager& manager, EntityIterator it);

	ViewIterator& operator++();

	ViewIterator operator++(int);

	value_type operator*() const;

	bool operator!=(const ViewIterator& other) const;
	bool operator==(const ViewIterator& other) const;

private:
	ComponentManager& m_manager;
	EntityIterator m_it;
};

} // namespace re::ecs

#include <ECS/View/Iterator/ViewIterator.inl>