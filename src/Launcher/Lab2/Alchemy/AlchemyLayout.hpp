#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/String.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Assets/AssetManager.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

// Подобрать названия, чтобы больше были похожи на названия модели игры
#include "AlchemyComponents.hpp"
#include "AlchemyGame.hpp"
#include "IngredientStorage.hpp"

#include <set>
#include <vector>

// Не давать переносить элементы на левую часть поля в библиотеку
// Удалять элементы, если пытаемся выбросить их в левой половине экрана
// Добавить слои для элементов
// Добавить картинки для всех элементов
class AlchemyLayout final : public re::Layout
{
	struct Layers
	{
		static constexpr int Background = -10;
		static constexpr int Default = 0;
		static constexpr int Items = 10;
		static constexpr int UI = 50;
		static constexpr int Dragging = 100;
	};

public:
	AlchemyLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
		, m_game(m_registry)
	{
	}

	void OnCreate() override
	{
		CreateUI();
		RefreshLibrary();
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* press = event.GetIf<re::Event::MouseButtonPressed>())
		{
			if (press->button == re::Mouse::Button::Left)
			{
				HandleMouseDown(m_window.ToWorldPos(press->position));
			}
		}
		if (const auto* release = event.GetIf<re::Event::MouseButtonReleased>())
		{
			if (release->button == re::Mouse::Button::Left)
			{
				HandleMouseUp();
			}
		}
		if (const auto* move = event.GetIf<re::Event::MouseMoved>())
		{
			HandleMouseMove(m_window.ToWorldPos(move->position));
		}
	}

private:
	static constexpr auto FONT_NAME = "Roboto.ttf";

	void CreateUI()
	{
		auto [width, height] = m_window.Size();
		auto font = GetAssetManager().Get<re::Font>(FONT_NAME);

		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>({ 0.f, 0.f })
			.Add<re::RectangleComponent>(re::Color::White, re::Vector2f{ 2.f, (float)height })
			.Add<re::ZIndexComponent>(Layers::UI);

		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>({ -275.f, -400.f })
			.Add<re::RectangleComponent>(re::Color::Gray, re::Vector2f{ 100.f, 30.f })
			.Add<re::TextComponent>("Sort", font, re::Color::White, 20.f)
			.Add<re::BoxColliderComponent>(re::Vector2f{ 100.f, 30.f }, re::Vector2f{ -275.f, -400.f })
			.Add<AlchemyButtonComponent>(AlchemyButtonComponent::Type::Sort)
			.Add<re::ZIndexComponent>(Layers::UI);

		re::Vector2f trashPos = { 400.f, 250.f };
		m_trashIcon = GetScene()
						  .CreateEntity()
						  .Add<re::TransformComponent>(trashPos)
						  .Add<re::BoxColliderComponent>(re::Vector2f{ 60.f, 60.f }, trashPos)
						  .Add<AlchemyButtonComponent>(AlchemyButtonComponent::Type::Clear)
						  .Add<re::ZIndexComponent>(Layers::UI)
						  .GetEntity();

		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>(trashPos, 45.f)
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 10.f, 60.f })
			.Add<re::ZIndexComponent>(Layers::UI);
		GetScene()
			.CreateEntity()
			.Add<re::TransformComponent>(trashPos, -45.f)
			.Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 10.f, 60.f })
			.Add<re::ZIndexComponent>(Layers::UI);

		m_logEntity = GetScene()
						  .CreateEntity()
						  .Add<re::TransformComponent>({ 0.f, 320.f })
						  .Add<re::TextComponent>("Start mixing elements!", font, re::Color::White, 24.f)
						  .Add<re::ZIndexComponent>(Layers::UI)
						  .GetEntity();
	}

	void RefreshLibrary()
	{
		for (const auto entity : m_libraryEntities)
		{
			if (GetScene().IsValid(entity))
			{
				GetScene().DestroyEntity(entity);
			}
		}

		m_libraryEntities.clear();

		constexpr re::Vector2f TOP_LEFT = { -450.f, -250.f };
		constexpr float SPACING = 120.f;

		int col = 0, row = 0;
		for (const int id : m_game.GetUnlockedElements())
		{
			const re::Vector2f pos = {
				TOP_LEFT.x + static_cast<float>(col) * SPACING,
				TOP_LEFT.y + static_cast<float>(row) * SPACING
			};
			auto entity = CreateElementEntity(id, pos, false);
			m_libraryEntities.push_back(entity.GetEntity());

			if (++col >= 4)
			{
				col = 0;
				row++;
			}
		}
	}

	re::ecs::EntityWrapper<re::ecs::Scene> CreateElementEntity(int id, re::Vector2f pos, bool isWorkspace)
	{
		const auto* def = m_registry.GetDef(id);
		auto font = GetAssetManager().Get<re::Font>(FONT_NAME);
		auto entity = GetScene().CreateEntity();

		static int zIndex = 0;

		entity
			.Add<re::TransformComponent>(pos)
			.Add<re::TextComponent>(def->name, font, re::Color::Black, 14.f)
			.Add<AlchemyElementComponent>(id, isWorkspace)
			.Add<re::BoxColliderComponent>(re::Vector2f{ 100.f, 100.f }, pos)
			.Add<re::ZIndexComponent>(Layers::Items + ++zIndex);

		if (isWorkspace)
		{
			entity.Add<DraggableComponent>();
		}

		const auto image = GetAssetManager().Get<re::Image>(re::file_system::AssetsPath(std::to_string(id) + ".png"));
		if (image)
		{
			image->Scale(100, 100);
			entity.Add<re::DynamicTextureComponent>(*image);
		}
		else
		{
			entity.Add<re::RectangleComponent>(def->color, re::Vector2f{ 100.f, 100.f });
		}

		return entity;
	}

	void HandleMouseDown(const re::Vector2f mousePos)
	{
		auto& scene = GetScene();

		for (auto&& [entity, btn, collider] : *scene.CreateView<AlchemyButtonComponent, re::BoxColliderComponent>())
		{ // Buttons
			if (collider.Contains(mousePos))
			{
				if (btn.type == AlchemyButtonComponent::Type::Sort)
				{
					m_game.SortUnlocked();
					RefreshLibrary();
				}

				return;
			}
		}

		re::ecs::Entity bestEntity = re::ecs::Entity::INVALID_ID;
		int maxZ = std::numeric_limits<int>::lowest();

		for (auto&& [entity, elem, collider, drag, zIndex] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent, DraggableComponent, re::ZIndexComponent>())
		{ // DragStart
			if (collider.Contains(mousePos))
			{
				if (zIndex.value > maxZ)
				{
					maxZ = zIndex.value;
					bestEntity = entity;
				}
			}
		}

		if (scene.IsValid(bestEntity))
		{
			scene.GetComponent<re::ZIndexComponent>(bestEntity).value = Layers::Dragging;

			m_draggedEntity = bestEntity;
			auto& drag = scene.GetComponent<DraggableComponent>(bestEntity);
			drag.isDragging = true;
			drag.dragOffset = scene.GetComponent<re::TransformComponent>(bestEntity).position - mousePos;

			return;
		}

		for (auto&& [entity, elem, collider] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent>())
		{ // Workspace
			if (!elem.isWorkspaceElement && collider.Contains(mousePos))
			{
				re::Vector2f spawnPos = mousePos;
				// if (spawnPos.x < 60.f)
				// {
				// 	spawnPos.x = 60.f;
				// }

				auto newEnt = CreateElementEntity(elem.elementId, spawnPos, true);
				m_draggedEntity = newEnt.GetEntity();

				auto& dragComp = newEnt.Get<DraggableComponent>();
				dragComp.isDragging = true;

				newEnt.Get<re::ZIndexComponent>().value = Layers::Dragging;

				return;
			}
		}
	}

	void HandleMouseMove(const re::Vector2f mousePos)
	{
		if (!GetScene().IsValid(m_draggedEntity))
		{
			return;
		}

		auto& tf = GetScene().GetComponent<re::TransformComponent>(m_draggedEntity);
		const auto& drag = GetScene().GetComponent<DraggableComponent>(m_draggedEntity);
		auto& col = GetScene().GetComponent<re::BoxColliderComponent>(m_draggedEntity);

		const re::Vector2f newPos = mousePos + drag.dragOffset;

		tf.position = newPos;
		col.position = tf.position;
	}

	void HandleMouseUp()
	{
		if (!GetScene().IsValid(m_draggedEntity))
		{
			return;
		}

		auto& scene = GetScene();
		const auto& dragCol = scene.GetComponent<re::BoxColliderComponent>(m_draggedEntity);

		if (dragCol.position.x < dragCol.size.x / 2)
		{
			scene.DestroyEntity(m_draggedEntity);
			return;
		}

		if (dragCol.Intersects(scene.GetComponent<re::BoxColliderComponent>(m_trashIcon)))
		{
			scene.DestroyEntity(m_draggedEntity);
		}
		else
		{
			for (auto&& [entity, elem, collider] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent>())
			{
				if (entity != m_draggedEntity && elem.isWorkspaceElement && dragCol.Intersects(collider))
				{
					const int idA = scene.GetComponent<AlchemyElementComponent>(m_draggedEntity).elementId;
					const int idB = elem.elementId;
					const re::Vector2f spawnPos = scene.GetComponent<re::TransformComponent>(entity).position;

					if (const auto result = m_game.Combine(idA, idB); result.success)
					{
						scene.DestroyEntity(m_draggedEntity);
						scene.DestroyEntity(entity);
						ProcessCraftResult(result, idA, idB, spawnPos);

						break;
					}
				}
			}
		}

		if (GetScene().IsValid(m_draggedEntity))
		{
			scene.GetComponent<re::ZIndexComponent>(m_draggedEntity).value = ++m_topZ;
		}

		m_draggedEntity = re::ecs::Entity::INVALID_ID;
	}

	void ProcessCraftResult(const AlchemyGame::CraftResult& res, const int idA, const int idB, re::Vector2f pos)
	{
		if (pos.x < 60.f)
		{
			pos.x = 60.f;
		}

		CreateElementEntity(res.resA, pos, true);
		if (res.resB != -1)
		{
			CreateElementEntity(res.resB, pos + re::Vector2f{ 20, 20 }, true);
		}

		const re::String msg = m_registry.GetDef(idA)->name
			+ " + "
			+ m_registry.GetDef(idB)->name
			+ " = "
			+ m_registry.GetDef(res.resA)->name;

		SetLog(msg);

		if (res.isNew)
		{
			RefreshLibrary();
			if (m_game.IsAllUnlocked())
			{
				SetLog("ALL ELEMENTS FOUND!");
			}
		}
	}

	void SetLog(re::String const& text)
	{
		if (GetScene().IsValid(m_logEntity))
		{
			GetScene().GetComponent<re::TextComponent>(m_logEntity).text = text;
		}
	}

	re::AssetManager& GetAssetManager()
	{
		return m_localAssetManager;
	}

private:
	int m_topZ = Layers::Items;

	re::render::IWindow& m_window;
	re::AssetManager m_localAssetManager;

	IngredientStorage m_registry;
	AlchemyGame m_game;

	re::ecs::Entity m_trashIcon = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_logEntity = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_draggedEntity = re::ecs::Entity::INVALID_ID;
	std::vector<re::ecs::Entity> m_libraryEntities;
};