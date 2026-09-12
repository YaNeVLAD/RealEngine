#pragma once

#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Vertex.hpp>

#include <cstdint>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API StaticMesh
{
public:
	StaticMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);
	~StaticMesh() = default;

	const std::vector<Vertex>& GetVertices() const { return m_vertices; }
	const std::vector<std::uint32_t>& GetIndices() const { return m_indices; }

	std::uint32_t GetIndexCount() const { return static_cast<std::uint32_t>(m_indices.size()); }
	bool IsTransparent() const { return m_isTransparent; }

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;

	bool m_isTransparent = false;

	// В будущем здесь могут храниться сырые указатели filament::VertexBuffer*
	// и filament::IndexBuffer*, если вы решите управлять их временем жизни прямо из меша,
	// а не через хэндлы EntityRenderHandles в системе.
};

} // namespace re