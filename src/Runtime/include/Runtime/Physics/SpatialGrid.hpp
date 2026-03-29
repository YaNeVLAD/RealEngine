#pragma once

#include <Core/Hash.hpp>
#include <Core/Math/Vector3.hpp>
#include <ECS/Entity/Entity.hpp>
#include <Runtime/Physics/AABB.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace re
{

class SpatialGrid
{
public:
	explicit SpatialGrid(float cellSize = 5.f)
		: m_cellSize(cellSize)
	{
	}

	void Clear()
	{
		m_cells.clear();
	}

	[[nodiscard]] std::vector<ecs::Entity> Query(AABB const& bounds) const
	{
		std::vector<ecs::Entity> result;

		auto [minX, minY, minZ] = static_cast<Vector3i>(bounds.min);
		auto [maxX, maxY, maxZ] = static_cast<Vector3i>(bounds.max);

		for (int x = minX; x <= maxX; ++x)
		{
			for (int y = minY; y <= maxY; ++y)
			{
				for (int z = minZ; z <= maxZ; ++z)
				{
					if (const auto it = m_cells.find(HashCoords(x, y, z)); it != m_cells.end())
					{
						result.insert(result.end(), it->second.begin(), it->second.end());
					}
				}
			}
		}

		std::ranges::sort(result);
		result.erase(std::ranges::unique(result).begin(), result.end());

		return result;
	}

	void Insert(ecs::Entity entity, AABB const& bounds)
	{
		auto [minX, minY, minZ] = static_cast<Vector3i>(bounds.min);
		auto [maxX, maxY, maxZ] = static_cast<Vector3i>(bounds.max);

		for (int x = minX; x <= maxX; ++x)
		{
			for (int y = minY; y <= maxY; ++y)
			{
				for (int z = minZ; z <= maxZ; ++z)
				{
					m_cells[HashCoords(x, y, z)].emplace_back(entity);
				}
			}
		}
	}

	void Remove(ecs::Entity entity, AABB const& oldBounds)
	{
		auto [minX, minY, minZ] = static_cast<Vector3i>(oldBounds.min);
		auto [maxX, maxY, maxZ] = static_cast<Vector3i>(oldBounds.max);

		for (int x = minX; x <= maxX; ++x)
		{
			for (int y = minY; y <= maxY; ++y)
			{
				for (int z = minZ; z <= maxZ; ++z)
				{
					uint64_t hash = HashCoords(x, y, z);
					if (const auto it = m_cells.find(hash); it != m_cells.end())
					{
						auto& cellEntities = it->second;
						std::erase(cellEntities, entity);

						if (cellEntities.empty())
						{
							m_cells.erase(it);
						}
					}
				}
			}
		}
	}

private:
	static Hash_t HashCoords(int x, int y, int z)
	{
		constexpr Hash_t HASH_X = 73856093;
		constexpr Hash_t HASH_Y = 19349663;
		constexpr Hash_t HASH_Z = 83492791;
		return (static_cast<Hash_t>(x) * HASH_X)
			^ (static_cast<Hash_t>(y) * HASH_Y)
			^ (static_cast<Hash_t>(z) * HASH_Z);
	}

	float m_cellSize;
	std::unordered_map<Hash_t, std::vector<ecs::Entity>> m_cells;
};

} // namespace re