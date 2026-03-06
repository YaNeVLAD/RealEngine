#include <RVM/Chunk.hpp>

namespace re::rvm
{

void Chunk::Write(const std::uint8_t byte)
{
	m_code.push_back(byte);
}

std::uint8_t Chunk::AddConstant(const Value value)
{
	m_constants.push_back(value);

	return static_cast<std::uint8_t>(m_constants.size() - 1);
}

const std::vector<std::uint8_t>& Chunk::GetCode() const
{
	return m_code;
}

const std::vector<Value>& Chunk::GetConstants() const
{
	return m_constants;
}

std::size_t Chunk::Size() const
{
	return m_code.size();
}

} // namespace re::rvm