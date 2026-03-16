#include <RenderCore/GLFW/IndexBuffer.hpp>

#include <glad/glad.h>

namespace re::render
{

IndexBuffer::IndexBuffer(const std::uint32_t* indices, const std::size_t count)
	: m_count(count)
	, m_capacity(count)
{
	const auto size = static_cast<GLsizeiptr>(count * sizeof(std::uint32_t));
	glCreateBuffers(1, &m_rendererId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

IndexBuffer::IndexBuffer(const std::uint32_t count)
	: m_count(count)
	, m_capacity(count)
{
	const auto size = static_cast<GLsizeiptr>(count * sizeof(std::uint32_t));
	glCreateBuffers(1, &m_rendererId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_rendererId);
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::SetData(const uint32_t* data, const std::size_t count, const std::size_t offset)
{
	m_count = count;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);

	const uint32_t sizeBytes = count * sizeof(uint32_t);
	const uint32_t offsetBytes = offset * sizeof(uint32_t);

	if (offset + count > m_capacity)
	{
		m_capacity = static_cast<std::size_t>(static_cast<double>(offset + count) * 1.5);
		const std::uint32_t capacityBytes = m_capacity * sizeof(std::uint32_t);

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, capacityBytes, nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offsetBytes, sizeBytes, data);
	}
	else
	{
		if (offset == 0)
		{
			const std::uint32_t capacityBytes = m_capacity * sizeof(std::uint32_t);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, capacityBytes, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offsetBytes, sizeBytes, data);
	}
}

std::size_t IndexBuffer::GetCount() const
{
	return m_count;
}

std::size_t IndexBuffer::GetCapacity() const
{
	return m_capacity;
}

} // namespace re::render
