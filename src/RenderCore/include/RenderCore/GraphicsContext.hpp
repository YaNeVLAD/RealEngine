#pragma once

#include <Core/Math/Color.hpp>

namespace re::render
{

class GraphicsContext
{
public:
	explicit GraphicsContext(void* windowHandle);

	static void OnBeforeWindowCreate();

	void OnAfterWindowCreate();

	void SetClearColor(Color color);

	void Clear();

	void SwapBuffers();

private:
	void* m_windowHandle{};
};

} // namespace re::render