#pragma once

#include <GUI/Export.hpp>

#include <RenderCore/Event.hpp>

namespace re::gui::Context
{

RE_GUI_API void Init(void* nativeWindowHandle);
RE_GUI_API void Shutdown();

RE_GUI_API void BeginFrame();
RE_GUI_API void EndFrame();

RE_GUI_API bool ProcessEvent(const Event& event);

RE_GUI_API void SetInteractive(bool interactive);

} // namespace re::gui::Context