#include <RenderCore/Event.hpp>

namespace re
{

template <typename TEventType>
Event::Event(TEventType const& data)
{
	m_data = data;
}

template <typename TEventType>
bool Event::Is() const
{
	return std::holds_alternative<TEventType>(m_data);
}

template <typename TEventType>
const TEventType* Event::GetIf() const
{
	return std::get_if<TEventType>(&m_data);
}

template <typename TVisitor>
decltype(auto) Event::Visit(TVisitor&& visitor) const
{
	return std::visit(std::forward<TVisitor>(visitor), m_data);
}

} // namespace re