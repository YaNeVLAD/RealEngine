#pragma once

#include <Core/Math/Vector2.hpp>
#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>

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

	struct MouseButtonPressed
	{
		Mouse::Button button{};
		Vector2i position;
	};

	struct MouseButtonReleased
	{
		Mouse::Button button{};
		Vector2i position;
	};

	struct MouseMoved
	{
		Vector2i position;
	};

	struct KeyPressed
	{
		Keyboard::Key key{};

		bool alt{};
		bool ctrl{};
		bool shift{};
	};

	struct KeyReleased
	{
		Keyboard::Key key{};

		bool alt{};
		bool ctrl{};
		bool shift{};
	};

	struct MouseWheelScrolled
	{
		float delta{};
		Vector2i position;
	};

	struct TextEntered
	{
		char32_t symbol{};
	};

	template <typename TEventType>
	explicit Event(TEventType const& data);

	template <typename TEventType>
	[[nodiscard]] bool Is() const;

	template <typename TEventType>
	[[nodiscard]] const TEventType* GetIf() const;

	template <typename TVisitor>
	decltype(auto) Visit(TVisitor&& visitor) const;

private:
	std::variant<
		Closed,
		Resized,
		KeyPressed,
		KeyReleased,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseWheelScrolled,
		TextEntered>
		m_data;
};

} // namespace re

#include <RenderCore/Event.inl>