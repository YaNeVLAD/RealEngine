#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/String.hpp>
#include <RenderCore/Assets/Font.hpp>
#include <RenderCore/Assets/Texture.hpp>
#include <RenderCore/Image.hpp>

namespace re
{

struct TransformComponent
{
	Vector2f position = { 0.f, 0.f };
	float rotation = 0.f;
	Vector2f scale = { 1.f, 1.f };
};

struct CircleComponent
{
	Color color = Color::White;
	float radius = 50.f;
};

struct RectangleComponent
{
	Color color = Color::White;
	Vector2f size = { 50.f, 50.f };
};

struct TextComponent
{
	String text;
	std::shared_ptr<Font> font;
	Color color = Color::White;
	float size = 24.f;
};

struct DynamicTextureComponent
{
	Image image;
	std::shared_ptr<Texture> texture = nullptr;
	bool isDirty = true;
};

struct CameraComponent
{
	float zoom = 1.f;
	bool isPrimal = true;
};

struct BoxColliderComponent
{
	Vector2f size;
	Vector2f position;

	[[nodiscard]] bool Contains(const Vector2f point) const
	{
		const float dx = std::abs(point.x - position.x);
		const float dy = std::abs(point.y - position.y);

		return dx <= size.x * 0.5f && dy <= size.y * 0.5f;
	}

	[[nodiscard]] bool Intersects(BoxColliderComponent const& rhs) const
	{
		const float dx = std::abs(position.x - rhs.position.x);
		const float dy = std::abs(position.y - rhs.position.y);

		const float sumHalfWidths = (size.x + rhs.size.x) * 0.5f;
		const float sumHalfHeights = (size.y + rhs.size.y) * 0.5f;

		return dx <= sumHalfWidths && dy <= sumHalfHeights;
	}
};

} // namespace re