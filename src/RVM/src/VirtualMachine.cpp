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

		case OpCode::Return: {
			if (!m_stack.empty())
			{
				std::cout << "Result: " << m_stack.back() << "\n";
				Pop();
			}
			return InterpreterResult::Success;
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