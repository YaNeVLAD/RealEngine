#pragma once

#include <glm/glm.hpp>

namespace re
{

struct LightData
{
	int type = 0;
	glm::vec3 position{ 0.f, 0.f, 0.f };
	glm::vec3 direction{ 0.f, -1.f, 0.f };

	glm::vec3 ambient{};
	glm::vec3 diffuse{};
	glm::vec3 specular{};

	float constant = 1.0f;
	float linear = 0.0f;
	float quadratic = 0.0f;

	float cutOff = 0.0f;
	float exponent = 0.0f;
};

} // namespace re