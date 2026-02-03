#pragma once

namespace re::render
{

class IWindow
{
public:
	virtual ~IWindow() = default;

	virtual bool IsOpen() const = 0;
	virtual void PollEvents() = 0;
	virtual void Clear() = 0;
	virtual void Display() = 0;
	virtual void* GetNativeHandle() = 0;
};

} // namespace re::render