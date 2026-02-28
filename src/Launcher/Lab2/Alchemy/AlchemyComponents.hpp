#pragma once

#include <Core/Math/Vector2.hpp>

struct AlchemyElementComponent
{
	int elementId = -1;
	bool isWorkspaceElement = false;
};

struct DraggableComponent
{
	bool isDragging = false;
	re::Vector2f dragOffset;
	re::Vector2f originalPosition;
};

struct AlchemyButtonComponent
{
	enum class Type
	{
		Sort,
		Clear
	} type;
};