#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/IWindow.hpp>

#include <SFML/Graphics/RenderWindow.hpp>

#include <string>

namespace re::render
{

class RE_RENDER_CORE_API SFMLWindow final : public IWindow
{
public:
	SFMLWindow(std::string const& title, unsigned width, unsigned height);

	bool IsOpen() const override;

	bool SetActive(bool active) override;

	std::optional<Event> PollEvent() override;

	void Clear() override;

	void Display() override;

	void* GetNativeHandle() override;

	sf::RenderWindow* GetSFMLWindow();

private:
	sf::RenderWindow m_window;
};

} // namespace re::render