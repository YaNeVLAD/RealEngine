#pragma once

#include <Core/Math/Vector2.hpp>

#include <concepts>
#include <variant>

namespace re
{

class Event
{
public:
	struct Closed
	{
	};

	struct Resized
	{
		Vector2u newSize;
	};

	template <typename TEventType>
	explicit Event(TEventType const& data);

	template <typename TEventType>
	[[nodiscard]] bool Is() const;

	template <typename TEventType>
	[[nodiscard]] TEventType* GetIf() const;

	template <typename TVisitor>
	decltype(auto) Visit(TVisitor&& visitor) const;

private:
	std::variant<
		Closed,
		Resized>
		m_data;
};

} // namespace re

#include <RenderCore/Event.inl>