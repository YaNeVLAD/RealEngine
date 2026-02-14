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

	int x = 0;
	int y = radius;
	int d = 3 - 2 * radius;

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

inline void DrawCircleWithSemiLines(
	re::Image& image,
	const re::Vector2i center,
	const int radius,
	const re::Color borderColor)
{
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

	int x = 0;
	int y = radius;
	int delta = 1 - 2 * radius;
	int error = 0;

	while (y >= x)
	{
		drawOctants(center.x, center.y, x, y);

		error = 2 * (delta + y) - 1;
		if (delta < 0 && error <= 0)
		{
			delta += 2 * ++x + 1;
			continue;
		}
		if (delta > 0 && error > 0)
		{
			delta -= 2 * --y + 1;
			continue;
		}
		delta += 2 * (++x - --y);
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

		DrawCircleWithSemiLines(
			image,
			{ WIDTH / 2 - 20, HEIGHT / 2 },
			5,
			re::Color::Red);

		DrawCircle(
			image,
			{ WIDTH / 2 + 20, HEIGHT / 2 },
			5,
			re::Color::Red);

		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>()
			.Add<re::DynamicTextureComponent>(image);
	}

private:
	re::render::IWindow& m_window;
};