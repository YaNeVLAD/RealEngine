#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/GLFW/BufferLayout.hpp>
#include <RenderCore/GLFW/IndexBuffer.hpp>
#include <RenderCore/GLFW/VertexBuffer.hpp>

#include <memory>
#include <vector>

namespace re::render
{

class RE_RENDER_CORE_API VertexArray
{
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;

	void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer, const BufferLayout& layout);
	void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

	const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const
	{
		return m_VertexBuffers;
	}
	const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}

private:
	std::uint32_t m_RendererID;
	std::uint32_t m_VertexBufferIndex = 0;
	std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

} // namespace re::render