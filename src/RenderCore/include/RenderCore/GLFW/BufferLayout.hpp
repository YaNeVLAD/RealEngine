#pragma once

#include <Core/String.hpp>
#include <RenderCore/Export.hpp>
#include <RenderCore/GLFW/BufferTypeTraits.hpp>

#include <vector>

namespace re::render
{

struct BufferElement
{
	String Name;
	std::uint32_t BaseType;
	std::uint32_t Count;
	std::size_t Size;
	std::size_t Offset;
	bool Normalized;

	template <typename T>
	static BufferElement Create(String const& name, const bool normalized = false)
	{
		return {
			name,
			BufferTypeTraits<T>::BaseType,
			BufferTypeTraits<T>::Count,
			sizeof(T),
			static_cast<std::size_t>(0),
			normalized
		};
	}
};

class RE_RENDER_CORE_API BufferLayout
{
public:
	BufferLayout();

	template <typename T>
	void Push(String const& name, bool normalized = false);

	[[nodiscard]] std::uint32_t GetStride() const;
	[[nodiscard]] const std::vector<BufferElement>& GetElements() const;

private:
	void CalculateOffsets();

private:
	std::vector<BufferElement> m_Elements;
	uint32_t m_Stride = 0;
};

template <typename T>
void BufferLayout::Push(String const& name, const bool normalized)
{
	m_Elements.emplace_back(BufferElement::Create<T>(name, normalized));
	m_Stride += sizeof(T);
	CalculateOffsets();
}

} // namespace re::render