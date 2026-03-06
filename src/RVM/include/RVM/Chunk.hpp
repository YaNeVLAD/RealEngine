#pragma once

#include <RVM/Export.hpp>

#include <RVM/Types.hpp>

#include <cstdint>
#include <vector>

namespace re::rvm
{

class RE_RVM_API Chunk
{
public:
	void Write(std::uint8_t byte);

	std::uint8_t AddConstant(Value value);

	[[nodiscard]] const std::vector<uint8_t>& GetCode() const;
	[[nodiscard]] const std::vector<Value>& GetConstants() const;
	[[nodiscard]] size_t Size() const;

private:
	std::vector<std::uint8_t> m_code;
	std::vector<Value> m_constants;
};

} // namespace re::rvm