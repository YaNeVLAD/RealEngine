#pragma once

#include <Core/Math/Color.hpp>
#include <Core/String.hpp>

#include <vector>

struct ElementDef
{
	int id;
	re::String name;
	re::Color color;
};

struct Recipe
{
	int inputA;
	int inputB;
	int resultA;
	int resultB;
};

class IngredientStorage
{
public:
	IngredientStorage()
	{
		Init();
	}

	const ElementDef* GetDef(const int id) const
	{
		// использовать find_if
		const auto it = std::ranges::find_if(m_definitions, [id](const ElementDef& def) {
			return def.id == id;
		});

		return it != m_definitions.end() ? &(*it) : nullptr;
	}

	const Recipe* FindRecipe(const int idA, const int idB) const
	{
		const auto it = std::ranges::find_if(m_recipes, [idA, idB](const Recipe& r) {
			return (r.inputA == idA && r.inputB == idB) || (r.inputA == idB && r.inputB == idA);
		});

		return it != m_recipes.end() ? &(*it) : nullptr;
	}

	const std::vector<ElementDef>& GetAllDefinitions() const
	{
		return m_definitions;
	}

private:
	void Init()
	{
		auto addEl = [&](const int id, re::String const& name, const re::Color col) {
			m_definitions.emplace_back(id, name, col);
		};

		addEl(0, "Огонь", re::Color::Red);
		addEl(1, "Вода", re::Color::Blue);
		addEl(2, "Земля", re::Color::Brown);
		addEl(3, "Воздух", re::Color::Cyan);

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
			{ 24, 10, 27, -1 }, // Болото + Энергия = Жизнь
			{ 27, 24, 28, -1 }, // Жизнь + Болото = Бактерии
		};
	}

	std::vector<ElementDef> m_definitions;
	std::vector<Recipe> m_recipes;
};