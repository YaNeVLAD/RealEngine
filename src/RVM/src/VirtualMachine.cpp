#include <RVM/VirtualMachine.hpp>

#include <Core/Assert.hpp>

#include <iostream>

#define READ_BYTE() (*m_ip++)
#define READ_CONSTANT() (m_chunk->GetConstants()[READ_BYTE()])
#define BINARY_OP(op_func, op_name)                                            \
	do                                                                         \
	{                                                                          \
		Value _b = Pop();                                                      \
		Value _a = Pop();                                                      \
		Value res = op_func(_a, _b);                                           \
		if (std::holds_alternative<std::monostate>(res))                       \
		{                                                                      \
			std::cerr << "Runtime Error: Invalid operands for " #op_name "\n"; \
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
		case OpCode::Add:   BINARY_OP([](const Value& a, const Value& b) { return a + b; }, ADD); break;
		case OpCode::Sub:   BINARY_OP([](const Value& a, const Value& b) { return a - b; }, SUB); break;
		case OpCode::Mul:   BINARY_OP([](const Value& a, const Value& b) { return a * b; }, MUL); break;
		case OpCode::Div:   BINARY_OP([](const Value& a, const Value& b) { return a * b; }, DIV); break;
		case OpCode::Equal: BINARY_OP([](const Value& a, const Value& b) { return OpEqual(a, b); }, EQUAL); break;
		case OpCode::Less:  BINARY_OP([](const Value& a, const Value& b) { return OpLess(a, b); }, LESS); break;
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

			CallFrame frame{};
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

		case OpCode::JmpIfFalse: {
			Value offsetVal = READ_CONSTANT();
			auto offset = std::get<std::int64_t>(offsetVal);
			if (!IsTruthy(Pop()))
			{
				m_ip = m_chunk->GetCode().data() + offset;
			}
			break;
		}

		case OpCode::Jmp: {
			Value offsetVal = READ_CONSTANT();
			m_ip = m_chunk->GetCode().data() + std::get<std::int64_t>(offsetVal);
			break;
		}

		case OpCode::CallIndirect: {
			std::uint8_t argCount = READ_BYTE();

			// Stack: [FuncAddr] [Arg1][Arg2]
			Value offsetVal = m_stack[m_stack.size() - 1 - argCount];
			auto offset = std::get<std::int64_t>(offsetVal);

			CallFrame frame{};
			frame.returnAddress = m_ip;

			frame.stackBase = m_stack.size() - argCount - 1;
			frame.localsBase = m_currentLocalsBase;

			m_callStack.push_back(frame);
			m_currentLocalsBase = m_variables.size();
			m_ip = m_chunk->GetCode().data() + offset;
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