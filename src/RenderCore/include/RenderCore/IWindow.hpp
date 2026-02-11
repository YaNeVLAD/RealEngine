#pragma once

#include <RenderCore/Event.hpp>

#include <optional>

namespace re::render
{

class IWindow
{
public:
	virtual ~IWindow() = default;

	virtual bool IsOpen() const = 0;
	virtual bool SetActive(bool active) = 0;
	virtual void SetTitle(std::string const& title) = 0;
	virtual std::optional<Event> PollEvent() = 0;
	virtual void Clear() = 0;
	virtual void Display() = 0;
	virtual void* GetNativeHandle() = 0;
};

} // namespace re::render