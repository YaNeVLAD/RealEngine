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

void SFMLWindow::PollEvents()
{
	while (const auto event = m_window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}
	}
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