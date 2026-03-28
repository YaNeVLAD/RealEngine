#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>

namespace re
{

struct Vertex
{
	Vector3f position;
	Vector3f normal = { 0.f, 0.f, 1.f };
	Color color = Color::White;
	Vector2f texCoord = { 0.f, 0.f };
	float texIndex = -1.0f;

	bool operator==(Vertex const&) const = default;
};

} // namespace re

template <>
struct std::hash<re::Vertex>
{
	std::size_t operator()(re::Vertex const& vertex) const noexcept
	{
		const std::size_t h1 = hash<float>()(vertex.position.x) ^ (hash<float>()(vertex.position.y) << 1);
		const std::size_t h2 = hash<float>()(vertex.normal.x) ^ (hash<float>()(vertex.normal.y) << 1);
		const std::size_t h3 = hash<float>()(vertex.texCoord.x);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};