#include <RenderCore/GLFW/StaticMesh.hpp>

namespace re
{

StaticMesh::StaticMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
	: m_indexCount(static_cast<std::uint32_t>(indices.size()))
{
	using namespace re::render;

	if (!vertices.empty())
	{
		m_isTransparent = vertices[0].color.a < 255;
	}

	m_vao = std::make_shared<VertexArray>();
	m_vao->Bind();

	m_vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex));
	m_ebo = std::make_shared<IndexBuffer>(indices.data(), indices.size());

	BufferLayout layout;
	layout.Push<Vector3f>("a_Position");
	layout.Push<Vector3f>("a_Normal");
	layout.Push<Color>("a_Color", true);
	layout.Push<Vector2f>("a_TexCoord");
	layout.Push<float>("a_TexIndex");
	layout.Push<Vector4i>("a_BoneIDs");
	layout.Push<Vector4f>("a_BoneWeights");

	m_vao->AddVertexBuffer(m_vbo, layout);
	m_vao->SetIndexBuffer(m_ebo);

	m_vao->Unbind();
	m_vbo->Unbind();
	m_ebo->Unbind();
}

render::VertexArray* StaticMesh::GetVAO() const
{
	return m_vao.get();
}

std::uint32_t StaticMesh::GetIndexCount() const
{
	return m_indexCount;
}

bool StaticMesh::IsTransparent() const
{
	return m_isTransparent;
}

} // namespace re