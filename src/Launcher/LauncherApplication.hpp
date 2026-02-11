#pragma once

#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>

#include <iostream>

#include "Components.hpp"
#include "HierarchySystem.hpp"
#include "JumpPhysicsSystem.hpp"
#include "Letters.hpp"

struct LettersLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene
			.AddSystem<JumpPhysicsSystem>()
			.WithRead<re::TransformComponent>()
			.WithWrite<GravityComponent>();

		scene
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>();

		scene.BuildSystemGraph();
		CreateLettersScene(scene);
	}
};

struct MenuLayout final : re::Layout
{
	using Layout::Layout;

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene.CreateEntity()
			.Add<re::TransformComponent>(re::Vector2f{}, 0.f, re::Vector2f{ 3.f, 1.f })
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 100, 100 });
	}
};

struct HouseLayout final : re::Layout
{
	HouseLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();
		scene
			.AddSystem<HierarchySystem>()
			.WithRead<ChildComponent>()
			.WithWrite<re::TransformComponent>();

		m_rootEntity = std::make_unique<re::ecs::EntityWrapper<re::ecs::Scene>>(scene.CreateEntity());
		m_rootEntity->Add<re::TransformComponent>();

		CreateHouseScene(scene);
		CalculateLocalBounds(scene);
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::MouseButtonPressed>())
		{
			const re::Vector2f worldMousePos = m_window.ToWorldPos(e->position);
			const auto& rootTransform = m_rootEntity->Get<re::TransformComponent>();

			const re::Vector2f localMousePos = worldMousePos - rootTransform.position;

			if (m_bounds.Contains(localMousePos))
			{
				m_isDragging = true;
				m_dragOffset = rootTransform.position - worldMousePos;
			}
		}

		if (event.Is<re::Event::MouseButtonReleased>())
		{
			m_isDragging = false;
		}

		if (const auto* e = event.GetIf<re::Event::MouseMoved>())
		{
			if (m_isDragging)
			{
				const re::Vector2f worldMousePos = m_window.ToWorldPos(e->position);
				auto& rootTransform = m_rootEntity->Get<re::TransformComponent>();
				rootTransform.position = worldMousePos + m_dragOffset;
			}
		}
	}

private:
	struct RectBounds
	{
		re::Vector2f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		re::Vector2f max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

		void Encapsulate(const re::Vector2f pos, const re::Vector2f size)
		{
			const auto [x, y] = size * 0.5f;
			min.x = std::min(min.x, pos.x - x);
			min.y = std::min(min.y, pos.y - y);
			max.x = std::max(max.x, pos.x + x);
			max.y = std::max(max.y, pos.y + y);
		}

		bool Contains(const re::Vector2f point) const
		{
			return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
		}

		void Reset()
		{
			min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
		}
	};

	void CreateHouseScene(re::ecs::Scene& scene) const
	{
		auto createPart = [&](
							  re::Vector2f localPos,
							  float localRot,
							  re::Color color,
							  re::Vector2f size,
							  const bool isCircle = false) {
			auto entity = scene.CreateEntity();
			entity.Add<ChildComponent>(m_rootEntity->GetEntity(), localPos, localRot);
			entity.Add<re::TransformComponent>();

			if (isCircle)
			{
				entity.Add<re::CircleComponent>(color, size.x / 2.f);
			}
			else
			{
				entity.Add<re::RectangleComponent>(color, size);
			}

			return entity;
		};

		const auto c_Wall = re::Color{ 245, 222, 179 };
		const auto c_Roof = re::Color{ 139, 69, 19 };
		const auto c_Chimney = re::Color{ 105, 105, 105 };
		const auto c_Door = re::Color{ 160, 82, 45 };
		const auto c_Window = re::Color{ 135, 206, 235 };
		const auto c_Frame = re::Color{ 255, 250, 250 };

		createPart({ 35.f, -90.f }, 0.f, c_Chimney, { 25.f, 50.f });

		createPart({ 0.f, 0.f }, 0.f, c_Wall, { 120.f, 100.f });

		// Replace with triangle
		createPart({ 0.f, -70.f }, 3.14159f / 4.f, c_Roof, { 100.f, 100.f });

		createPart({ 0.f, 30.f }, 0.f, c_Door, { 36.f, 60.f });
		createPart({ 12.f, 30.f }, 0.f, re::Color::Black, { 6.f, 6.f }, true); // Ручка

		auto makeWindow = [&](const float xOffset) {
			createPart({ xOffset, -10.f }, 0.f, c_Frame, { 34.f, 34.f }); // Рама
			createPart({ xOffset, -10.f }, 0.f, c_Window, { 28.f, 28.f }); // Стекло

			createPart({ xOffset, -10.f }, 0.f, c_Frame, { 28.f, 4.f });
			createPart({ xOffset, -10.f }, 0.f, c_Frame, { 4.f, 28.f });
		};

		makeWindow(-35.f);
		makeWindow(35.f);
	}

	void CalculateLocalBounds(re::ecs::Scene& scene)
	{
		m_bounds.Reset();
		for (auto&& [entity, child] : *scene.CreateView<ChildComponent>())
		{
			if (child.parentEntity != m_rootEntity->GetEntity())
			{
				continue;
			}

			re::Vector2f size = { 0.f, 0.f };

			if (scene.HasComponent<re::RectangleComponent>(entity))
			{
				size = scene.GetComponent<re::RectangleComponent>(entity).size;
			}
			else if (scene.HasComponent<re::CircleComponent>(entity))
			{
				const float r = scene.GetComponent<re::CircleComponent>(entity).radius;
				size = { r * 2.f, r * 2.f };
			}

			if (child.localRotation != 0.f)
			{
				const float maxSize = std::max(size.x, size.y) * 1.4142f;
				size = { maxSize, maxSize };
			}

			m_bounds.Encapsulate(child.localPosition, size);
		}
	}

	re::render::IWindow& m_window;

	std::unique_ptr<re::ecs::EntityWrapper<re::ecs::Scene>> m_rootEntity = nullptr;

	bool m_isDragging = false;
	re::Vector2f m_dragOffset;

	const float houseWidth = 100.f;
	const float houseHeight = 100.f;

	RectBounds m_bounds;
};

class LauncherApplication final : public re::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
	}

	void OnStart() override
	{
		AddLayout<LettersLayout>();
		AddLayout<MenuLayout>();
		AddLayout<HouseLayout>(Window());

		SwitchLayout<MenuLayout>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		static float timeAccumulator = 0.0f;
		static int frames = 0;

		timeAccumulator += deltaTime;
		frames++;

		if (timeAccumulator >= 1.0f)
		{
			const auto fps = std::format("FPS: {} | ms: {}",
				frames,
				timeAccumulator / static_cast<float>(frames) * 1000.0f);

			Window().SetTitle(fps);

			frames = 0;
			timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{

			if (e->key == re::Keyboard::Key::Num1)
			{
				SwitchLayout<MenuLayout>();
			}
			if (e->key == re::Keyboard::Key::Num2)
			{
				SwitchLayout<LettersLayout>();
			}
			if (e->key == re::Keyboard::Key::Num3)
			{
				SwitchLayout<HouseLayout>();
			}
		}
	}

	void OnStop() override
	{
	}
};