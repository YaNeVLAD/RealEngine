#pragma once

#include <Core/Math/Color.hpp>
#include <RenderCore/Texture.hpp>

namespace re
{

struct Material
{
	Color ambient = Color::White;
	Color diffuse = Color::White;
	Color specular = Color::Gray;
	Color emission = Color::Transparent;
	float shininess = 32.0f;

	std::shared_ptr<Texture> texture = nullptr;

	bool operator==(const Material& rhs) const = default;

	bool operator<(Material const& rhs) const
	{
		return std::tie(ambient, diffuse, specular, emission, shininess, texture)
			< std::tie(rhs.ambient, rhs.diffuse, rhs.specular, rhs.emission, rhs.shininess, rhs.texture);
	}
};

} // namespace re