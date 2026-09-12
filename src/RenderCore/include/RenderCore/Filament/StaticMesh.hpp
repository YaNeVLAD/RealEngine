#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Material.hpp>
#include <RenderCore/Vertex.hpp>

#include <cstdint>
#include <utility>
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

	void SetMaterial(Material material) { m_material = std::move(material); }
	[[nodiscard]] const Material& GetMaterial() const { return m_material; }

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;

	Material m_material;

	bool m_isTransparent = false;
};

} // namespace re