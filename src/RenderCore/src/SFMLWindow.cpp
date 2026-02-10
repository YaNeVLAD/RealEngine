#include <RenderCore/Window/SFMLWindow.hpp>

#include <SFML/Graphics.hpp>

namespace re::render
{

SFMLWindow::SFMLWindow(std::string const& title, const unsigned width, const unsigned height)
{
	m_window.create(sf::VideoMode(sf::Vector2u{ width, height }), title);
	m_window.setVerticalSyncEnabled(true);
}

bool SFMLWindow::IsOpen() const
{
	return m_window.isOpen();
}

bool SFMLWindow::SetActive(const bool active)
{
	return m_window.setActive(active);
}

std::optional<Event> SFMLWindow::PollEvent()
{
	if (const auto event = m_window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			return Event(Event::Closed{});
		}
		if (const auto* resized = event->getIf<sf::Event::Resized>())
		{
			return Event(Event::Resized{ resized->size.x, resized->size.y });
		}
		if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			const auto key = static_cast<Keyboard::Key>(keyPressed->code);
			const auto alt = keyPressed->alt;
			const auto ctrl = keyPressed->control;
			const auto shift = keyPressed->shift;

			return Event(Event::KeyPressed{ key, alt, ctrl, shift });
		}
		if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
		{
			const auto key = static_cast<Keyboard::Key>(keyReleased->code);
			const auto alt = keyReleased->alt;
			const auto ctrl = keyReleased->control;
			const auto shift = keyReleased->shift;

			return Event(Event::KeyReleased{ key, alt, ctrl, shift });
		}
		if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
		{
			const auto button = static_cast<Mouse::Button>(mousePressed->button);
			const auto position = Vector2i{ mousePressed->position.x, mousePressed->position.y };
			return Event(Event::MouseButtonPressed{ button, position });
		}
		if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
		{
			const auto button = static_cast<Mouse::Button>(mouseReleased->button);
			const auto position = Vector2i{ mouseReleased->position.x, mouseReleased->position.y };
			return Event(Event::MouseButtonReleased{ button, position });
		}
		if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
		{
			const auto position = Vector2i{ mouseMoved->position.x, mouseMoved->position.y };
			return Event(Event::MouseMoved{ position });
		}

		return PollEvent();
	}

	return std::nullopt;
}

void SFMLWindow::Clear()
{
	m_window.clear();
}

void SFMLWindow::Display()
{
	m_window.display();
}

void* SFMLWindow::GetNativeHandle()
{
	return m_window.getNativeHandle();
}

sf::RenderWindow* SFMLWindow::GetSFMLWindow()
{
	return &m_window;
}

} // namespace re::render