#pragma once

#include "IngredientStorage.hpp"

#include <algorithm>
#include <vector>

class AlchemyGame
{
public:
	struct CraftResult
	{
		bool success = false;
		int resA = -1;
		int resB = -1;
		bool isNew = false;
	};

	explicit AlchemyGame(const IngredientStorage& storage)
		: m_storage(storage)
	{
		UnlockElement(0); // Огонь
		UnlockElement(1); // Вода
		UnlockElement(2); // Земля
		UnlockElement(3); // Воздух
	}

	bool UnlockElement(const int id)
	{
		if (id == -1)
		{
			return false;
		}

		if (const auto it = std::ranges::find(m_unlockedIds, id); it == m_unlockedIds.end())
		{
			m_unlockedIds.push_back(id);
			return true;
		}

		return false;
	}

	CraftResult Combine(const int idA, const int idB)
	{
		const auto* recipe = m_storage.FindRecipe(idA, idB);
		if (!recipe)
		{
			return { false };
		}

		const bool newA = UnlockElement(recipe->resultA);
		const bool newB = UnlockElement(recipe->resultB);

		return { true, recipe->resultA, recipe->resultB, newA || newB };
	}

	const std::vector<int>& GetUnlockedElements() const
	{
		return m_unlockedIds;
	}

	bool IsAllUnlocked() const
	{
		return m_unlockedIds.size() >= m_storage.GetAllDefinitions().size();
	}

	void SortUnlocked()
	{
		std::ranges::sort(m_unlockedIds, [this](const int a, const int b) {
			const auto* defA = m_storage.GetDef(a);
			const auto* defB = m_storage.GetDef(b);

			if (!defA || !defB)
			{
				return defA != nullptr;
			}

			return defA->name < defB->name;
		});
	}

private:
	const IngredientStorage& m_storage;
	std::vector<int> m_unlockedIds;
};