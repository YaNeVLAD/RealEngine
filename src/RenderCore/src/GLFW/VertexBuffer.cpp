#include <RenderCore/GLFW/VertexBuffer.hpp>

#include <glad/glad.h>

namespace re::render
{

VertexBuffer::VertexBuffer(const void* vertices, const std::size_t size)
	: m_size(size)
{
	const auto sz = static_cast<GLsizeiptr>(size);
	glCreateBuffers(1, &m_renderId);
	glBindBuffer(GL_ARRAY_BUFFER, m_renderId);
	glBufferData(GL_ARRAY_BUFFER, sz, vertices, GL_STATIC_DRAW);
}

VertexBuffer::VertexBuffer(const std::size_t size)
	: m_size(size)
{
	const auto sz = static_cast<GLsizeiptr>(size);
	glCreateBuffers(1, &m_renderId);
	glBindBuffer(GL_ARRAY_BUFFER, m_renderId);
	glBufferData(GL_ARRAY_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_renderId);
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_renderId);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void* vertices, const std::size_t size, const std::size_t offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, m_renderId);

	if (offset + size > m_size)
	{
		m_size = static_cast<std::size_t>(static_cast<double>(offset + size) * 1.5);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_size), nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLintptr>(size), vertices);
	}
	else
	{
		if (offset == 0)
		{
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_size), nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLintptr>(size), vertices);
	}
}

std::size_t VertexBuffer::GetSize() const
{
	return m_size;
}

} // namespace re::render
