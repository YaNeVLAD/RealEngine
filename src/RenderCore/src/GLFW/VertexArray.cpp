#include <RenderCore/GLFW/VertexArray.hpp>

#include <glad/glad.h>

namespace re::render
{

VertexArray::VertexArray()
{
	glCreateVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::Bind() const
{
	glBindVertexArray(m_RendererID);
}

void VertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer, const BufferLayout& layout)
{
	glBindVertexArray(m_RendererID);
	vertexBuffer->Bind();

	for (const auto& element : layout.GetElements())
	{
		glEnableVertexAttribArray(m_VertexBufferIndex);
		glVertexAttribPointer(
			m_VertexBufferIndex,
			static_cast<int>(element.Count),
			element.BaseType,
			element.Normalized ? GL_TRUE : GL_FALSE,
			static_cast<int>(layout.GetStride()),
			reinterpret_cast<const void*>(static_cast<uintptr_t>(element.Offset)));
		m_VertexBufferIndex++;
	}

	m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
	glBindVertexArray(m_RendererID);
	indexBuffer->Bind();
	m_IndexBuffer = indexBuffer;
}

} // namespace re::render