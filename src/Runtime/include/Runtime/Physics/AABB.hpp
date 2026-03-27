#pragma once

#include <Core/Math/Vector3.hpp>

namespace re
{

struct AABB
{
	Vector3f min;
	Vector3f max;

	[[nodiscard]] static AABB FromCenterSize(const Vector3f& center, const float size)
	{
		const auto sz = Vector3f(size, size, size);

		return { center - sz, center + sz };
	}

	[[nodiscard]] static AABB FromCenterExtents(const Vector3f& center, const Vector3f& halfExtents)
	{
		return { center - halfExtents, center + halfExtents };
	}

	[[nodiscard]] static AABB FromCenterSize(const Vector3f& center, const Vector3f& size)
	{
		const auto halfSize = size * 0.5f;

		return { center - halfSize, center + halfSize };
	}

	[[nodiscard]] Vector3f Center() const
	{
		return (min + max) * 0.5f;
	}

	[[nodiscard]] Vector3f Size() const
	{
		return max - min;
	}

	[[nodiscard]] Vector3f HalfExtents() const
	{
		return (max - min) * 0.5f;
	}

	[[nodiscard]] bool Intersects(const AABB& rhs) const
	{
		return (min.x <= rhs.max.x && max.x >= rhs.min.x)
			&& (min.y <= rhs.max.y && max.y >= rhs.min.y)
			&& (min.z <= rhs.max.z && max.z >= rhs.min.z);
	}

	[[nodiscard]] AABB Swept(const Vector3f& velocity) const
	{
		AABB swept = *this;

		// clang-format off
		if (velocity.x > 0.0f) swept.max.x += velocity.x; else swept.min.x += velocity.x;
		if (velocity.y > 0.0f) swept.max.y += velocity.y; else swept.min.y += velocity.y;
		if (velocity.z > 0.0f) swept.max.z += velocity.z; else swept.min.z += velocity.z;
		// clang-format on

		return swept;
	}
};

} // namespace re