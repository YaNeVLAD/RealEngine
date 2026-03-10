#pragma once

#include <RVM/Export.hpp>

#include <Core/HashedString.hpp>
#include <RVM/Chunk.hpp>
#include <RVM/Types.hpp>

#include <cstdint>
#include <functional>
#include <vector>

namespace re::rvm
{

enum class InterpreterResult : std::uint8_t
{
	Success = 0,
	CompileError,
	RuntimeError,
};

using NativeFn = std::function<Value(std::vector<Value> const&)>;

class RE_RVM_API VirtualMachine
{
public:
	VirtualMachine();

	InterpreterResult Interpret(const Chunk& chunk);

	void RegisterNative(String const& name, NativeFn fn);

private:
	InterpreterResult Run();

	Value Pop();
	void Push(const Value& value);

private:
	const Chunk* m_chunk = nullptr;
	const std::uint8_t* m_ip = nullptr;

	std::vector<Value> m_stack;
	std::vector<Value> m_variables;

	std::vector<const std::uint8_t*> m_callStack;
	std::unordered_map<Hash_t, NativeFn> m_natives;
};

} // namespace re::rvm