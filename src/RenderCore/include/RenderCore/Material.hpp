#pragma once

#include <Core/Math/Color.hpp>
#include <RenderCore/Texture.hpp>

namespace re
{

enum class MaterialWorkflow
{
	BlinnPhong,
	PBR
};

struct Material
{
	MaterialWorkflow workflow = MaterialWorkflow::BlinnPhong;

	// Common

	Color ambientColor{ 50, 50, 50, 255 };
	Color albedoColor{ 255, 255, 255, 255 };
	Color emissionColor{ 0, 0, 0, 255 };

	std::shared_ptr<Texture> albedoMap = nullptr;
	std::shared_ptr<Texture> normalMap = nullptr;
	std::shared_ptr<Texture> emissionMap = nullptr;

	// Blinn-Phong

	Color specularColor{ 255, 255, 255, 255 };
	float shininess = 32.0f;

	// PBR

	float metallicFactor = 0.0f;
	float roughnessFactor = 1.0f;
	std::shared_ptr<Texture> metallicRoughnessMap = nullptr;
	std::shared_ptr<Texture> ambientOcclusionMap = nullptr;

	bool operator==(const Material& rhs) const = default;

	bool operator<(Material const& rhs) const
	{
		return std::tie(
				   workflow, ambientColor, albedoColor, emissionColor, specularColor, shininess,
				   metallicFactor, roughnessFactor,
				   albedoMap, normalMap, emissionMap, metallicRoughnessMap, ambientOcclusionMap)
			< std::tie(
				rhs.workflow, ambientColor, rhs.albedoColor, rhs.emissionColor, rhs.specularColor, rhs.shininess,
				rhs.metallicFactor, rhs.roughnessFactor,
				rhs.albedoMap, rhs.normalMap, rhs.emissionMap, rhs.metallicRoughnessMap, rhs.ambientOcclusionMap);
	}
};

} // namespace re