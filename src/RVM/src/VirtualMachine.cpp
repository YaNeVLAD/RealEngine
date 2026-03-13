#include <RVM/VirtualMachine.hpp>

#include <Core/Assert.hpp>

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
	m_natives[name.Hash()] = std::move(fn);
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
			const std::size_t actualIndex = m_currentLocalsBase + slot;
			if (actualIndex >= m_variables.size())
			{
				Push(Null);
			}
			else [[likely]]
			{
				Push(m_variables[actualIndex]);
			}
			break;
		}

		case OpCode::SetLocal: {
			const std::uint8_t slot = READ_BYTE();
			Value val = Pop();

			const std::size_t actualIndex = m_currentLocalsBase + slot;
			if (m_variables.size() <= actualIndex)
			{
				m_variables.resize(actualIndex + 1);
			}
			m_variables[actualIndex] = std::move(val);
			break;
		}

		case OpCode::Call: {
			Value offsetVal = READ_CONSTANT();
			auto offset = std::get<std::int64_t>(offsetVal);

			std::uint8_t argCount = READ_BYTE();

			CallFrame frame;
			frame.returnAddress = m_ip;

			frame.stackBase = m_stack.size() - argCount;
			frame.localsBase = m_currentLocalsBase;

			m_callStack.push_back(frame);

			m_currentLocalsBase = m_variables.size();

			m_ip = m_chunk->GetCode().data() + offset;
			break;
		}

		case OpCode::Native: {
			Value nameVal = READ_CONSTANT();
			auto funcName = std::get<String>(nameVal);

			std::uint8_t argCount = READ_BYTE();
			auto it = m_natives.find(funcName.Hash());
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
			Value retVal = Null;
			if (!m_stack.empty())
			{
				retVal = Pop();
			}

			if (!m_callStack.empty())
			{
				const auto& [returnAddress, stackBase, localsBase] = m_callStack.back();
				m_ip = returnAddress;

				m_stack.resize(stackBase);

				m_variables.resize(m_currentLocalsBase);
				m_currentLocalsBase = localsBase;

				m_callStack.pop_back();

				Push(retVal);
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
	RE_ASSERT(!m_stack.empty(), "You should not call VirtualMachine::Pop on empty stack");

	const Value val = m_stack.back();
	m_stack.pop_back();

	return val;
}

void VirtualMachine::Push(const Value& value)
{
	m_stack.push_back(value);
}

} // namespace re::rvm