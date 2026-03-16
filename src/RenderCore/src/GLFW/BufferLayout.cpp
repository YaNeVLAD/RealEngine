#include <RenderCore/GLFW/BufferLayout.hpp>

namespace re::render
{

BufferLayout::BufferLayout() = default;

std::uint32_t BufferLayout::GetStride() const
{
	return m_Stride;
}

const std::vector<BufferElement>& BufferLayout::GetElements() const
{
	return m_Elements;
}

void BufferLayout::CalculateOffsets()
{
	std::size_t offset = 0;
	for (auto& element : m_Elements)
	{
		element.Offset = offset;
		offset += element.Size;
	}
}

} // namespace re::render