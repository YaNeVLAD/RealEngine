#pragma once

#include <RenderCore/IWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

inline void DrawCircle(
	re::Image& image,
	const re::Vector2i center,
	const int radius,
	const re::Color borderColor)
{
	int x = 0;
	int y = radius;
	int d = 3 - 2 * radius;

	auto drawOctants = [&](const int xc, const int yc, const int _x, const int _y) {
		image.SetPixel(xc + _x, yc + _y, borderColor);
		image.SetPixel(xc - _x, yc + _y, borderColor);
		image.SetPixel(xc + _x, yc - _y, borderColor);
		image.SetPixel(xc - _x, yc - _y, borderColor);
		image.SetPixel(xc + _y, yc + _x, borderColor);
		image.SetPixel(xc - _y, yc + _x, borderColor);
		image.SetPixel(xc + _y, yc - _x, borderColor);
		image.SetPixel(xc - _y, yc - _x, borderColor);
	};

	while (y >= x)
	{
		drawOctants(center.x, center.y, x, y);
		x++;

		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}
	}
}

struct CircleLayout final : re::Layout
{
	CircleLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		re::Image image;
		constexpr auto WIDTH = 100;
		constexpr auto HEIGHT = 100;
		image.Resize(WIDTH, HEIGHT, re::Color::White);
		DrawCircle(
			image,
			{ WIDTH / 2, HEIGHT / 2 },
			40,
			re::Color::Red);

		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>()
			.Add<re::DynamicTextureComponent>(image);
	}

private:
	re::render::IWindow& m_window;
};