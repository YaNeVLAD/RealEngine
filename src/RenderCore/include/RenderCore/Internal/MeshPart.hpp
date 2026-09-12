#pragma once

#include <RenderCore/Material.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

namespace re::render
{

struct MeshPart
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	Material material;
};

} // namespace re::render