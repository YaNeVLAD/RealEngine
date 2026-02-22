#pragma once

#include <Core/String.hpp>
#include <RenderCore/Event.hpp>
#include <RenderCore/Image.hpp>

#include <optional>

namespace re::render
{

class IWindow
{
public:
	virtual ~IWindow() = default;

	virtual bool IsOpen() const = 0;
	virtual bool SetActive(bool active) = 0;
	virtual void SetTitle(String const& title) = 0;
	virtual void SetIcon(Image const& image) = 0;
	virtual void SetVSyncEnabled(bool enabled) = 0;
	virtual void OnUpdate() = 0;
	virtual Vector2f ToWorldPos(Vector2i const& pixelPos) = 0;
	virtual std::optional<Event> PollEvent() = 0;
	virtual Vector2u Size() = 0;
	virtual void Clear() = 0;
	virtual void Display() = 0;
	virtual void* GetNativeHandle() = 0;
};

} // namespace re::render