#include <ECS/View/Iterator/ViewIterator.hpp>

namespace re::ecs
{

template <bool IsConst, typename... TComponents>
ViewIterator<IsConst, TComponents...>::ViewIterator(ComponentManager& manager, EntityIterator it)
	: m_manager(manager)
	, m_it(it)
{
}

template <bool IsConst, typename... TComponents>
ViewIterator<IsConst, TComponents...>& ViewIterator<IsConst, TComponents...>::operator++()
{
	++m_it;
	return *this;
}

template <bool IsConst, typename... TComponents>
ViewIterator<IsConst, TComponents...> ViewIterator<IsConst, TComponents...>::operator++(int)
{
	ViewIterator tmp = *this;
	++(*this);
	return tmp;
}

template <bool IsConst, typename... TComponents>
typename ViewIterator<IsConst, TComponents...>::value_type ViewIterator<IsConst, TComponents...>::operator*() const
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

template <bool IsConst, typename... TComponents>
bool ViewIterator<IsConst, TComponents...>::operator!=(const ViewIterator& other) const
{
	return m_it != other.m_it;
}

template <bool IsConst, typename... TComponents>
bool ViewIterator<IsConst, TComponents...>::operator==(const ViewIterator& other) const
{
	return m_it == other.m_it;
}

} // namespace re::ecs