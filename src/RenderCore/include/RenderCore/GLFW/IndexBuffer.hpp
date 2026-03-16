#pragma once

#include <RenderCore/Export.hpp>

#include <cstddef>
#include <cstdint>

namespace re::render
{

class RE_RENDER_CORE_API IndexBuffer
{
public:
	IndexBuffer(const std::uint32_t* indices, std::size_t count);

	explicit IndexBuffer(std::uint32_t count);

	~IndexBuffer();

	void Bind() const;
	void Unbind() const;

	void SetData(const std::uint32_t* data, std::size_t count, std::size_t offset = 0);

	std::size_t GetCount() const;

	std::size_t GetCapacity() const;

private:
	std::uint32_t m_rendererId{};
	std::size_t m_count;
	std::size_t m_capacity;
};

} // namespace re::render