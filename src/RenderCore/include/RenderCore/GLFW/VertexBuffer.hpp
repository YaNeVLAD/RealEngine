#pragma once

#include <RenderCore/Export.hpp>

#include <cstddef>
#include <cstdint>

namespace re::render
{

class RE_RENDER_CORE_API VertexBuffer
{
public:
	VertexBuffer(const void* vertices, std::size_t size);

	explicit VertexBuffer(std::size_t size);

	~VertexBuffer();

	void Bind() const;
	void Unbind() const;

	void SetData(const void* vertices, std::size_t size, std::size_t offset = 0);

	std::size_t GetSize() const;

private:
	std::uint32_t m_renderId{};
	std::size_t m_size{};
};

} // namespace re::render
