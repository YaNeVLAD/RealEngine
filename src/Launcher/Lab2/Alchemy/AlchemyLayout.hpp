#pragma once

#include "Runtime/Assets/AssetManager.hpp"

#include <algorithm>
#include <format>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/String.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

// Компонент, помечающий сущность как элемент игры
struct AlchemyElementComponent
{
	int elementId = -1;
	bool isWorkspaceElement = false; // true - на поле экспериментов, false - в библиотеке
};

// Компонент для объектов, которые можно перетаскивать
struct DraggableComponent
{
	bool isDragging = false;
	re::Vector2f dragOffset;
	re::Vector2f originalPosition; // Чтобы вернуть на место, если что-то пошло не так
};

// Компонент-кнопка (сортировка, очистка)
struct AlchemyButtonComponent
{
	enum class Type
	{
		Sort,
		Clear
	} type;
};

class AlchemyLayout final : public re::Layout
{
	// Описание элемента
	struct ElementDef
	{
		int id;
		re::String name;
		re::Color color;
	};

	// Описание рецепта
	struct Recipe
	{
		int inputA;
		int inputB;
		int resultA;
		int resultB; // -1 если нет второго результата
	};

public:
	AlchemyLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		InitDatabase();

		// Добавляем 4 базовых элемента
		UnlockElement(0); // Огонь
		UnlockElement(1); // Вода
		UnlockElement(2); // Земля
		UnlockElement(3); // Воздух

		CreateUI();
		RefreshLibrary();
	}

	void OnUpdate(re::core::TimeDelta dt) override
	{
		// Логика обновления, если потребуется анимация
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
		else if (const auto* release = event.GetIf<re::Event::MouseButtonReleased>())
		{
			if (release->button == re::Mouse::Button::Left)
			{
				HandleMouseUp();
			}
		}
		else if (const auto* move = event.GetIf<re::Event::MouseMoved>())
		{
			HandleMouseMove(m_window.ToWorldPos(move->position));
		}
	}

private:
	// --- Инициализация данных ---
	void InitDatabase()
	{
		// Цвета генерируем или задаем вручную для красоты
		auto addEl = [&](const int id, re::String const& name, const re::Color col) {
			m_definitions.emplace_back(ElementDef{ id, name, col });
		};

		// Базовые
		addEl(0, "Огонь", re::Color::Red);
		addEl(1, "Вода", re::Color::Blue);
		addEl(2, "Земля", re::Color::Brown);
		addEl(3, "Воздух", re::Color::Cyan);

		// Производные (по таблице)
		addEl(4, "Пар", re::Color(220, 220, 220));
		addEl(5, "Лава", re::Color(255, 69, 0));
		addEl(6, "Пыль", re::Color(169, 169, 169));
		addEl(7, "Порох", re::Color(47, 79, 79));
		addEl(8, "Взрыв", re::Color(255, 215, 0));
		addEl(9, "Дым", re::Color(105, 105, 105));
		addEl(10, "Энергия", re::Color(238, 130, 238));
		addEl(11, "Камень", re::Color(112, 128, 144));
		addEl(12, "Буря", re::Color(70, 130, 180));
		addEl(13, "Металл", re::Color(192, 192, 192));
		addEl(14, "Электричество", re::Color(255, 255, 0));
		addEl(15, "Водород", re::Color(240, 248, 255));
		addEl(16, "Кислород", re::Color(240, 255, 255));
		addEl(17, "Озон", re::Color(176, 224, 230));
		addEl(18, "Грязь", re::Color(101, 67, 33));
		addEl(19, "Гейзер", re::Color(224, 255, 255));
		addEl(20, "Паровой котел", re::Color(184, 134, 11));
		addEl(21, "Давление", re::Color(255, 140, 0));
		addEl(22, "Вулкан", re::Color(178, 34, 34));
		addEl(23, "Гремучий газ", re::Color(255, 228, 181));
		addEl(24, "Болото", re::Color(85, 107, 47));
		addEl(25, "Спирт", re::Color(250, 250, 210));
		addEl(26, "Коктейль Молотова", re::Color(255, 99, 71));
		addEl(27, "Жизнь", re::Color(50, 205, 50));
		addEl(28, "Бактерии", re::Color(154, 205, 50));
		addEl(29, "Водка", re::Color(255, 255, 255));

		// Рецепты (из таблицы на картинке)
		m_recipes = {
			{ 0, 1, 4, -1 }, // Огонь + Вода = Пар
			{ 0, 2, 5, -1 }, // Огонь + Земля = Лава
			{ 3, 2, 6, -1 }, // Воздух + Земля = Пыль
			{ 0, 6, 7, -1 }, // Огонь + Пыль = Порох
			{ 7, 0, 8, 9 }, // Порох + Огонь = Взрыв + Дым
			{ 3, 0, 10, -1 }, // Воздух + Огонь = Энергия
			{ 5, 1, 4, 11 }, // Лава + Вода = Пар + Камень
			{ 3, 10, 12, -1 }, // Воздух + Энергия = Буря
			{ 0, 11, 13, -1 }, // Огонь + Камень = Металл
			{ 13, 10, 14, -1 }, // Металл + Энергия = Электричество
			{ 14, 1, 15, 16 }, // Электр + Вода = Водород + Кислород
			{ 14, 16, 17, -1 }, // Электр + Кислород = Озон
			{ 6, 1, 18, -1 }, // Пыль + Вода = Грязь
			{ 4, 2, 19, -1 }, // Пар + Земля = Гейзер
			{ 4, 13, 20, -1 }, // Пар + Металл = Паровой котел
			{ 20, 4, 21, -1 }, // Пар.котел + Пар = Давление
			{ 5, 21, 22, -1 }, // Лава + Давление = Вулкан
			{ 15, 16, 23, -1 }, // Водород + Кислород = Гремучий газ
			{ 1, 2, 24, -1 }, // Вода + Земля = Болото
			{ 0, 1, 25, -1 }, // Огонь + Вода = Спирт (Второй рецепт! См логику)
			{ 25, 0, 26, -1 }, // Спирт + Огонь = К.Молотова
			{ 24, 10, 27, -1 }, // Болото + Энергия = Жизнь
			{ 27, 24, 28, -1 }, // Жизнь + Болото = Бактерии
			{ 25, 1, 29, -1 } // Спирт + Вода = Водка
		};
	}

	// --- UI и создание сущностей ---
	void CreateUI()
	{
		auto font = GetAssetManager().Get<re::Font>("assets/Roboto.ttf");

		// Разделительная линия
		GetScene().CreateEntity().Add<re::TransformComponent>(re::Vector2f{ 300.f, 340.f }) // x=300 - граница
			.Add<re::RectangleComponent>(re::Color::Black, re::Vector2f{ 2.f, 680.f });

		// Кнопка сортировки (Слева вверху)
		GetScene().CreateEntity().Add<re::TransformComponent>(re::Vector2f{ 150.f, 30.f }).Add<re::RectangleComponent>(re::Color::Gray, re::Vector2f{ 100.f, 30.f }).Add<re::TextComponent>("Sort", font, re::Color::White, 20.f).Add<re::BoxColliderComponent>(re::Vector2f{ 100.f, 30.f }, re::Vector2f{ 150.f, 30.f }).Add<AlchemyButtonComponent>(AlchemyButtonComponent::Type::Sort);

		// Кнопка удаления (Справа внизу, красный крест)
		auto deleteBtn = GetScene().CreateEntity();
		deleteBtn.Add<re::TransformComponent>(re::Vector2f{ 950.f, 600.f })
			.Add<re::BoxColliderComponent>(re::Vector2f{ 60.f, 60.f }, re::Vector2f{ 950.f, 600.f })
			.Add<AlchemyButtonComponent>(AlchemyButtonComponent::Type::Clear);

		// Визуализация крестика
		m_trashIcon = deleteBtn.GetEntity();

		// Рисуем крестик двумя прямоугольниками
		GetScene().CreateEntity().Add<re::TransformComponent>(re::Vector2f{ 950.f, 600.f }, 45.f).Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 10.f, 60.f });
		GetScene().CreateEntity().Add<re::TransformComponent>(re::Vector2f{ 950.f, 600.f }, -45.f).Add<re::RectangleComponent>(re::Color::Red, re::Vector2f{ 10.f, 60.f });

		// Лог сообщений (Внизу)
		m_logEntity = GetScene().CreateEntity().Add<re::TransformComponent>(re::Vector2f{ 600.f, 650.f }).Add<re::TextComponent>("Start mixing elements!", font, re::Color::Black, 24.f).GetEntity();
	}

	void RefreshLibrary()
	{
		// Удаляем старые элементы библиотеки
		for (const auto entity : m_libraryEntities)
		{
			if (GetScene().IsValid(entity))
			{
				GetScene().DestroyEntity(entity);
			}
		}
		m_libraryEntities.clear();
		GetScene().ConfirmChanges();

		auto font = GetAssetManager().Get<re::Font>("assets/Roboto.ttf");
		constexpr float startX = 60.f;
		constexpr float startY = 80.f;
		constexpr float paddingY = 60.f;
		constexpr float paddingX = 120.f; // 2 колонки
		int col = 0;
		int row = 0;

		for (const int id : m_unlockedElements)
		{
			const auto& def = GetDef(id);
			const float x = startX + (float)col * paddingX;
			const float y = startY + (float)row * paddingY;

			auto entity = CreateElementEntity(id, { x, y }, false);
			m_libraryEntities.push_back(entity.GetEntity());

			col++;
			if (col > 1)
			{
				col = 0;
				row++;
			}
		}
	}

	re::ecs::EntityWrapper<re::ecs::Scene> CreateElementEntity(int id, re::Vector2f pos, bool isWorkspace)
	{
		auto font = GetAssetManager().Get<re::Font>("assets/Roboto.ttf");
		const auto& def = GetDef(id);

		auto entity = GetScene().CreateEntity();
		entity.Add<re::TransformComponent>(pos)
			.Add<re::RectangleComponent>(def.color, re::Vector2f{ 80.f, 40.f })
			.Add<re::TextComponent>(def.name, font, re::Color::Black, 14.f)
			.Add<AlchemyElementComponent>(id, isWorkspace)
			.Add<re::BoxColliderComponent>(re::Vector2f{ 80.f, 40.f }, pos);

		if (isWorkspace)
		{
			entity.Add<DraggableComponent>();
		}

		return entity;
	}

	// --- Input Handling ---

	void HandleMouseDown(const re::Vector2f mousePos)
	{
		auto& scene = GetScene();

		// 1. Проверяем UI кнопки
		for (auto&& [entity, btn, collider] : *scene.CreateView<AlchemyButtonComponent, re::BoxColliderComponent>())
		{
			if (collider.Contains(mousePos))
			{
				if (btn.type == AlchemyButtonComponent::Type::Sort)
				{
					SortLibrary();
				}
				else if (btn.type == AlchemyButtonComponent::Type::Clear)
				{
					// Кнопка очистки работает как дроп-зона, клик по ней ничего не делает
				}
				return;
			}
		}

		// 2. Проверяем элементы на рабочем столе (верхний слой первым, но у нас нет Z-index, берем любой)
		for (auto&& [entity, elem, collider, draggable] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent, DraggableComponent>())
		{
			if (collider.Contains(mousePos))
			{
				m_draggedEntity = entity;
				draggable.isDragging = true;
				draggable.dragOffset = scene.GetComponent<re::TransformComponent>(entity).position - mousePos;
				draggable.originalPosition = scene.GetComponent<re::TransformComponent>(entity).position;
				return;
			}
		}

		// 3. Проверяем библиотеку (спавним копию)
		for (auto&& [entity, elem, collider] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent>())
		{
			if (!elem.isWorkspaceElement && collider.Contains(mousePos))
			{
				// Создаем новый на поле экспериментов прямо под мышкой
				auto newEnt = CreateElementEntity(elem.elementId, mousePos, true);
				m_draggedEntity = newEnt.GetEntity();
				auto& [isDragging, dragOffset, originalPosition] = newEnt.Get<DraggableComponent>();
				isDragging = true;
				dragOffset = { 0.f, 0.f };
				originalPosition = mousePos;
				return;
			}
		}
	}

	void HandleMouseMove(const re::Vector2f mousePos)
	{
		if (GetScene().IsValid(m_draggedEntity))
		{
			auto& transform = GetScene().GetComponent<re::TransformComponent>(m_draggedEntity);
			const auto& draggable = GetScene().GetComponent<DraggableComponent>(m_draggedEntity);
			auto& collider = GetScene().GetComponent<re::BoxColliderComponent>(m_draggedEntity);

			transform.position = mousePos + draggable.dragOffset;
			collider.position = transform.position; // Обновляем коллайдер для проверок
		}
	}

	void HandleMouseUp()
	{
		if (!GetScene().IsValid(m_draggedEntity))
			return;

		auto& scene = GetScene();
		const auto& draggedElem = scene.GetComponent<AlchemyElementComponent>(m_draggedEntity);
		const auto& draggedTransform = scene.GetComponent<re::TransformComponent>(m_draggedEntity);
		const auto& draggedCollider = scene.GetComponent<re::BoxColliderComponent>(m_draggedEntity);

		// Сброс флага
		scene.GetComponent<DraggableComponent>(m_draggedEntity).isDragging = false;

		// 1. Проверка на удаление (пересечение с корзиной)
		// Упрощенно проверяем расстояние до центра корзины или пересечение AABB
		if (Intersects(draggedCollider, scene.GetComponent<re::BoxColliderComponent>(m_trashIcon)))
		{
			scene.DestroyEntity(m_draggedEntity);
			scene.ConfirmChanges();
			m_draggedEntity = re::ecs::Entity::INVALID_ID;
			return;
		}

		// 2. Проверка на реакцию (пересечение с другим элементом на столе)
		bool reacted = false;
		re::ecs::Entity otherEntity = re::ecs::Entity::INVALID_ID;

		for (auto&& [entity, elem, collider] : *scene.CreateView<AlchemyElementComponent, re::BoxColliderComponent>())
		{
			if (entity == m_draggedEntity)
			{
				continue;
			}
			if (!elem.isWorkspaceElement)
			{
				continue;
			}

			if (Intersects(draggedCollider, collider))
			{
				otherEntity = entity;
				reacted = TryCombine(m_draggedEntity, otherEntity);
				if (reacted)
				{
					break;
				}
			}
		}

		if (reacted)
		{
			// Элементы удаляются внутри TryCombine
		}
		else
		{
			// Просто оставляем на месте
		}

		m_draggedEntity = re::ecs::Entity::INVALID_ID;
	}

	// --- Game Logic ---

	bool TryCombine(const re::ecs::Entity a, const re::ecs::Entity b)
	{
		auto& scene = GetScene();
		const int idA = scene.GetComponent<AlchemyElementComponent>(a).elementId;
		const int idB = scene.GetComponent<AlchemyElementComponent>(b).elementId;
		const auto pos = scene.GetComponent<re::TransformComponent>(b).position;

		const Recipe* match = nullptr;
		// Ищем рецепт (A+B или B+A)
		// Обработка конфликтов (например, Огонь+Вода): берем первый найденный в векторе
		for (const auto& r : m_recipes)
		{
			if ((r.inputA == idA && r.inputB == idB) || (r.inputA == idB && r.inputB == idA))
			{
				match = &r;
				break;
			}
		}

		if (match)
		{
			// Удаляем реагенты
			scene.DestroyEntity(a);
			scene.DestroyEntity(b);
			scene.ConfirmChanges();

			// Создаем продукты
			SpawnOnWorkspace(match->resultA, pos);
			if (match->resultB != -1)
			{
				SpawnOnWorkspace(match->resultB, pos + re::Vector2f{ 20.f, 20.f });
			}

			// Логика открытия новых
			bool newDiscovered = false;
			if (UnlockElement(match->resultA))
			{
				newDiscovered = true;
			}
			if (match->resultB != -1)
			{
				if (UnlockElement(match->resultB))
				{
					newDiscovered = true;
				}
			}

			// Сообщение
			re::String msg = GetDef(idA).name + " + " + GetDef(idB).name + " = " + GetDef(match->resultA).name;
			if (match->resultB != -1)
			{
				msg += ", " + GetDef(match->resultB).name;
			}
			SetLog(msg);

			if (newDiscovered)
			{
				RefreshLibrary();
				CheckWinCondition();
			}

			return true;
		}

		return false;
	}

	void SpawnOnWorkspace(const int id, const re::Vector2f pos)
	{
		CreateElementEntity(id, pos, true);
	}

	bool UnlockElement(const int id)
	{
		if (std::ranges::find(m_unlockedElements, id) == m_unlockedElements.end())
		{
			m_unlockedElements.push_back(id);
			return true;
		}
		return false;
	}

	void SortLibrary()
	{
		std::ranges::sort(m_unlockedElements, [&](const int a, const int b) {
			return GetDef(a).name < GetDef(b).name;
		});
		RefreshLibrary();
	}

	void CheckWinCondition()
	{
		if (m_unlockedElements.size() >= m_definitions.size())
		{
			SetLog("CONGRATULATIONS! ALL ELEMENTS FOUND!");
		}
	}

	void SetLog(re::String const& text)
	{
		if (GetScene().IsValid(m_logEntity))
		{
			GetScene().GetComponent<re::TextComponent>(m_logEntity).text = text;
		}
	}

	// --- Helpers ---

	const ElementDef& GetDef(const int id)
	{
		for (const auto& d : m_definitions)
		{
			if (d.id == id)
			{
				return d;
			}
		}
		return m_definitions[0];
	}

	re::AssetManager& GetAssetManager()
	{
		// Небольшой хак для доступа к менеджеру, так как Layout его не хранит публично,
		// но Application хранит ресурсы. В данном примере мы создаем Font каждый раз через менеджер,
		// который сам внутри кэширует. AssetManager создается внутри Application.
		// В коде LauncherApplication.hpp видно, что Layout создается с Application&.
		// Но Layout API не дает прямого доступа к AssetManager приложения.
		// Однако, в LauncherApplication.hpp есть пример MenuLayout, который создает свой m_manager.
		// Мы сделаем так же.
		return m_localAssetManager;
	}

	static bool Intersects(const re::BoxColliderComponent& a, const re::BoxColliderComponent& b)
	{
		float aLeft = a.position.x - a.size.x / 2;
		float aRight = a.position.x + a.size.x / 2;
		float aTop = a.position.y - a.size.y / 2;
		float aBottom = a.position.y + a.size.y / 2;

		float bLeft = b.position.x - b.size.x / 2;
		float bRight = b.position.x + b.size.x / 2;
		float bTop = b.position.y - b.size.y / 2;
		float bBottom = b.position.y + b.size.y / 2;

		return (aLeft < bRight && aRight > bLeft && aTop < bBottom && aBottom > bTop);
	}

private:
	re::render::IWindow& m_window;
	re::AssetManager m_localAssetManager;

	std::vector<ElementDef> m_definitions;
	std::vector<Recipe> m_recipes;
	std::vector<int> m_unlockedElements;

	std::vector<re::ecs::Entity> m_libraryEntities;
	re::ecs::Entity m_trashIcon = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_logEntity = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_draggedEntity = re::ecs::Entity::INVALID_ID;
};