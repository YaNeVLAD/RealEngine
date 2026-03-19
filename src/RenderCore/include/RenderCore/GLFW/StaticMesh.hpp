#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/GLFW/IndexBuffer.hpp>
#include <RenderCore/GLFW/VertexArray.hpp>
#include <RenderCore/GLFW/VertexBuffer.hpp>
#include <RenderCore/Vertex.hpp>

#include <memory>
#include <vector>

namespace re
{

class RE_RENDER_CORE_API StaticMesh
{
public:
	StaticMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);

	render::VertexArray* GetVAO() const;
	std::uint32_t GetIndexCount() const;
	bool IsTransparent() const;

private:
	std::shared_ptr<render::VertexArray> m_vao;
	std::shared_ptr<render::VertexBuffer> m_vbo;
	std::shared_ptr<render::IndexBuffer> m_ebo;

	std::uint32_t m_indexCount = 0;
	bool m_isTransparent = false;
};

} // namespace re