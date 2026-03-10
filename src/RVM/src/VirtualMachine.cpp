#include <RVM/VirtualMachine.hpp>

#include <cassert>
#include <iostream>

#define READ_BYTE() (*m_ip++)
#define READ_CONSTANT() (m_chunk->GetConstants()[READ_BYTE()])
#define BINARY_OP(op_char)                                                     \
	do                                                                         \
	{                                                                          \
		Value b = Pop();                                                       \
		Value a = Pop();                                                       \
		Value res = a op_char b;                                               \
		if (std::holds_alternative<std::monostate>(res))                       \
		{                                                                      \
			std::cerr << "Runtime Error: Invalid operands for " #op_char "\n"; \
			return InterpreterResult::RuntimeError;                            \
		}                                                                      \
		Push(res);                                                             \
	} while (false)

namespace re::rvm
{

VirtualMachine::VirtualMachine()
{
	m_stack.reserve(256);
}

InterpreterResult VirtualMachine::Interpret(const Chunk& chunk)
{
	m_chunk = &chunk;
	m_ip = m_chunk->GetCode().data();
	m_stack.clear();
	m_variables.clear();

	return Run();
}

void VirtualMachine::RegisterNative(String const& name, NativeFn fn)
{
	const auto hash = HashedU32String::Value(name.Data(), name.Length());
	m_natives[hash] = std::move(fn);
}

InterpreterResult VirtualMachine::Run()
{
	for (;;)
	{
		const std::uint8_t instruction = READ_BYTE();
		switch (static_cast<OpCode>(instruction))
		{
		case OpCode::Const: {
			Value constant = READ_CONSTANT();
			Push(constant);
			break;
		}

			// clang-format off
		case OpCode::Add: BINARY_OP(+); break;
		case OpCode::Sub: BINARY_OP(-); break;
		case OpCode::Mul: BINARY_OP(*); break;
		case OpCode::Div: BINARY_OP(/); break;
			// clang-format on

		case OpCode::Pop: {
			Pop();
			break;
		}

		case OpCode::GetLocal: {
			const std::uint8_t slot = READ_BYTE();
			if (slot >= m_variables.size())
			{ // Error
				Push(std::monostate{});
			}
			else [[likely]]
			{
				Push(m_variables[slot]);
			}
			break;
		}

		case OpCode::SetLocal: {
			std::uint8_t slot = READ_BYTE();
			Value val = Pop();

			if (m_variables.size() <= slot)
			{
				m_variables.resize(slot + 1);
			}
			m_variables[slot] = val;
			break;
		}

		case OpCode::Call: {
			Value offsetVal = READ_CONSTANT();
			auto offset = std::get<std::int64_t>(offsetVal);

			m_callStack.push_back(m_ip);
			m_ip = m_chunk->GetCode().data() + offset;
			break;
		}

		case OpCode::Native: {
			Value nameVal = READ_CONSTANT();
			auto funcName = std::get<String>(nameVal);

			std::uint8_t argCount = READ_BYTE();
			auto hash = HashedU32String::Value(funcName.Data(), funcName.Length());
			auto it = m_natives.find(hash);
			if (it == m_natives.end())
			{
				throw std::runtime_error("Unknown native function " + funcName);
			}

			std::vector<Value> args(argCount);
			for (int i = argCount - 1; i >= 0; --i)
			{
				args[i] = Pop();
			}

			Value result = it->second(args);
			Push(result);
			break;
		}

		case OpCode::Return: {
			if (!m_callStack.empty())
			{
				m_ip = m_callStack.back();
				m_callStack.pop_back();
			}
			else
			{
				return InterpreterResult::Success;
			}
			break;
		}

		default:
			return InterpreterResult::RuntimeError;
		}
	}
}

Value VirtualMachine::Pop()
{
	assert(!m_stack.empty());

	const Value val = m_stack.back();
	m_stack.pop_back();

	return val;
}

void VirtualMachine::Push(const Value& value)
{
	m_stack.push_back(value);
}

} // namespace re::rvm