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

	std::shared_ptr<Texture> albedoMap = nullptr;
	std::shared_ptr<Texture> normalMap = nullptr;
	std::shared_ptr<Texture> emissionMap = nullptr;

	bool operator==(const Material& rhs) const = default;

	bool operator<(Material const& rhs) const
	{
		return std::tie(ambient, diffuse, specular, emission, shininess, albedoMap, normalMap, emissionMap)
			< std::tie(rhs.ambient, rhs.diffuse, rhs.specular, rhs.emission, rhs.shininess, rhs.albedoMap, rhs.normalMap, rhs.emissionMap);
	}
};

} // namespace re