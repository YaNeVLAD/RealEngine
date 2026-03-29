#pragma once

#include <Core/Math/Color.hpp>

namespace re
{

struct Material
{
	Color ambient = { 255, 255, 255, 255 };
	Color diffuse = { 255, 255, 255, 255 };
	Color specular = { 128, 128, 128, 255 };
	Color emission = { 0, 0, 0, 255 };
	float shininess = 32.0f;

	bool operator==(Material const& rhs) const = default;

	bool operator<(Material const& rhs) const
	{
		return std::tie(ambient.r, ambient.g, ambient.b, diffuse.r, specular.r, emission.r, shininess)
			< std::tie(rhs.ambient.r, rhs.ambient.g, rhs.ambient.b, rhs.diffuse.r, rhs.specular.r, rhs.emission.r, rhs.shininess);
	}
};

} // namespace re