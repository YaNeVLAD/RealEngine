#pragma once

#include <RVM/Export.hpp>

#include <Core/Assets/IAsset.hpp>
#include <Core/String.hpp>
#include <RVM/Types.hpp>

#include <cstdint>
#include <vector>

namespace re::rvm
{

class RE_RVM_API Chunk final : public IAsset
{
public:
	void Write(std::uint8_t byte);

	std::uint8_t AddConstant(Value value);

	[[nodiscard]] const std::vector<uint8_t>& GetCode() const;
	[[nodiscard]] const std::vector<Value>& GetConstants() const;
	[[nodiscard]] std::size_t Size() const;

	void Patch(std::size_t offset, std::uint8_t byte);

	bool SaveToFile(String const& filepath) const;

	bool LoadFromFile(String const& filepath) override;

private:
	std::vector<std::uint8_t> m_code;
	std::vector<Value> m_constants;
};

} // namespace re::rvm