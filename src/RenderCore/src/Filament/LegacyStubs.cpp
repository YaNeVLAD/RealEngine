#include <Core/String.hpp>
#include <RenderCore/GLFW/IndexBuffer.hpp>
#include <RenderCore/GLFW/VertexArray.hpp>
#include <RenderCore/GLFW/VertexBuffer.hpp>

namespace re::render
{

// IndexBuffer
IndexBuffer::IndexBuffer(const unsigned int*, unsigned long long) {}
IndexBuffer::~IndexBuffer() {}

// BufferLayout
BufferLayout::BufferLayout() {}
void BufferLayout::CalculateOffsets() {}

// VertexBuffer
VertexBuffer::VertexBuffer(unsigned long long) {}
VertexBuffer::~VertexBuffer() {}
void VertexBuffer::SetData(const void*, unsigned long long, unsigned long long) {}

// VertexArray
VertexArray::VertexArray() {}
VertexArray::~VertexArray() {}
void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>&, const BufferLayout&) {}
void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>&) {}

} // namespace re::render