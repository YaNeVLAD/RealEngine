#pragma once

#include <Core/Math/Color.hpp>
#include <RenderCore/IWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

#include "../../Components.hpp"
#include "../../HierarchySystem.hpp"

// Поправить поведение при ресайзе окна
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

		scene.BuildSystemGraph();

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
							  re::Vector2f size) {
			auto entity = scene.CreateEntity();
			entity.Add<ChildComponent>(m_rootEntity->GetEntity(), localPos, localRot);
			entity.Add<re::TransformComponent>();

			entity.Add<re::RectangleComponent>(color, size);

			return entity;
		};

		constexpr auto wallColor = re::Color{ 245, 222, 179 };
		constexpr auto rootColor = re::Color{ 139, 69, 19 };
		constexpr auto chimneyColor = re::Color{ 105, 105, 105 };
		constexpr auto doorColor = re::Color{ 160, 82, 45 };
		constexpr auto windowColor = re::Color{ 135, 206, 235 };
		constexpr auto frameColor = re::Color{ 255, 250, 250 };

		createPart({ 35.f, -90.f }, 0.f, chimneyColor, { 25.f, 50.f });
		createPart({ 0.f, -49.f }, 45, rootColor, { 105.f, 105.f });

		createPart({ 0.f, 0.f }, 0.f, wallColor, { 149.f, 100.f });

		createPart({ 0.f, 20.f }, 0.f, doorColor, { 30.f, 60.f });

		auto makeWindow = [&](const float xOffset) {
			createPart({ xOffset, -10.f }, 0.f, frameColor, { 34.f, 34.f });
			createPart({ xOffset, -10.f }, 0.f, windowColor, { 28.f, 28.f });

			createPart({ xOffset, -10.f }, 0.f, frameColor, { 28.f, 4.f });
			createPart({ xOffset, -10.f }, 0.f, frameColor, { 4.f, 28.f });
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