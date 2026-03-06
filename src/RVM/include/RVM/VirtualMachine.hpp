#pragma once

#include <RVM/Export.hpp>

#include <RVM/Chunk.hpp>
#include <RVM/Types.hpp>

#include <cstdint>
#include <vector>

namespace re::rvm
{

enum class InterpreterResult : std::uint8_t
{
	Success = 0,
	CompileError,
	RuntimeError,
};

class RE_RVM_API VirtualMachine
{
public:
	VirtualMachine();

	InterpreterResult Interpret(const Chunk& chunk);

private:
	InterpreterResult Run();

	Value Pop();
	void Push(const Value& value);

private:
	const Chunk* m_chunk = nullptr;
	const std::uint8_t* m_ip = nullptr;

	std::vector<Value> m_stack;
};

} // namespace re::rvm