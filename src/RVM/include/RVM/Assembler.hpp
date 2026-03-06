#pragma once

#include <RVM/Export.hpp>

#include <RVM/Chunk.hpp>

#include <string>
#include <unordered_map>

namespace re::rvm
{

class RE_RVM_API Assembler
{
public:
	bool Compile(const std::string& source, Chunk& outChunk);
};

} // namespace re::rvm