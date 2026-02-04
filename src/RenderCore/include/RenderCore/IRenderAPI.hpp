#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>

#include <cstdint>

namespace re::render
{

class IRenderAPI
{
public:
	virtual ~IRenderAPI() = default;

	virtual void SetViewport(core::Vector2f topLeft, core::Vector2f size) = 0;
	virtual void SetCamera(core::Vector2f center, core::Vector2f size) = 0;
	virtual void SetClearColor(core::Color const& color) = 0;
	virtual void Flush() = 0;
	virtual void DrawQuad(core::Vector2f const& pos, core::Vector2f const& size, core::Color const& color) = 0;
};

} // namespace re::render