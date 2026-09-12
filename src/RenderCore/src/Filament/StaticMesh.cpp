#include <RenderCore/Filament/StaticMesh.hpp>

namespace re
{

StaticMesh::StaticMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
	: m_vertices(vertices)
	, m_indices(indices)
{
}

} // namespace re