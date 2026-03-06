#pragma once

#include <RVM/Export.hpp>

#include "Core/HashedString.hpp"
#include <RVM/Chunk.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace re::rvm
{

class RE_RVM_API Assembler
{
public:
	bool Compile(const std::string& source, Chunk& outChunk);

private:
	std::unordered_map<HashedString, std::uint8_t> m_locals;
	std::uint8_t m_localsCount{};

	std::uint8_t DeclareVariable(const std::string& name);

	std::optional<std::uint8_t> ResolveVariable(const std::string& name);
};

} // namespace re::rvm