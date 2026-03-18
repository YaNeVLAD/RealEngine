#include <RVM/VirtualMachine.hpp>

#include <Core/Assert.hpp>
#include <Core/HashedString.hpp>
#include <Core/Meta/TypeInfo.hpp>

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

void VirtualMachine::RegisterType(TypeInfoPtr typeInfo)
{
	m_types[typeInfo->name.Hash()] = std::move(typeInfo);
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

		case OpCode::New: {
			Value classNameVal = READ_CONSTANT();
			auto className = std::get<String>(classNameVal);

			auto it = m_types.find(className.Hash());
			if (it == m_types.end())
			{
				std::cerr << "Runtime Error: Unknown class '" << className.ToString() << "'\n";
				return InterpreterResult::RuntimeError;
			}

			auto instance = std::make_shared<Instance>(it->second);
			Push(instance);
			break;
		}

		case OpCode::GetProperty: {
			Value propNameVal = READ_CONSTANT();
			auto propNameHash = std::get<String>(propNameVal).Hash();

			Value objVal = Pop();
			if (!std::holds_alternative<std::shared_ptr<Instance>>(objVal))
			{
				std::cerr << "Runtime Error: Only instances have properties.\n";
				return InterpreterResult::RuntimeError;
			}

			auto instance = std::get<std::shared_ptr<Instance>>(objVal);
			auto it = instance->typeInfo->fieldIndexes.find(propNameHash);

			if (it == instance->typeInfo->fieldIndexes.end())
			{
				std::cerr << "Runtime Error: Undefined property.\n";
				return InterpreterResult::RuntimeError;
			}

			Push(instance->fields[it->second]);
			break;
		}

		case OpCode::SetProperty: {
			Value propNameVal = READ_CONSTANT();
			auto propNameHash = std::get<String>(propNameVal).Hash();

			Value valueToSet = Pop();
			Value objVal = Pop();

			if (!std::holds_alternative<std::shared_ptr<Instance>>(objVal))
			{
				std::cerr << "Runtime Error: Only instances have properties.\n";
				return InterpreterResult::RuntimeError;
			}

			auto instance = std::get<std::shared_ptr<Instance>>(objVal);
			auto it = instance->typeInfo->fieldIndexes.find(propNameHash);

			if (it == instance->typeInfo->fieldIndexes.end())
			{
				std::cerr << "Runtime Error: Undefined property.\n";
				return InterpreterResult::RuntimeError;
			}

			instance->fields[it->second] = std::move(valueToSet);
			break;
		}

		case OpCode::TypeOf: {
			if (Value objVal = Pop(); std::holds_alternative<std::shared_ptr<Instance>>(objVal))
			{
				auto instance = std::get<std::shared_ptr<Instance>>(objVal);
				Push(instance->typeInfo);
			}
			else
			{
				// TODO: Implement base TypeInfo for predefined VM types
				Push(Null);
			}
			break;
		}

		case OpCode::DefType: {
			Value countVal = Pop();
			Value nameVal = Pop();

			if (!std::holds_alternative<Int>(countVal) || !std::holds_alternative<String>(nameVal))
			{
				std::cerr << "Runtime Error: Invalid arguments for DefType\n";
				return InterpreterResult::RuntimeError;
			}

			auto fieldCount = std::get<Int>(countVal);
			auto className = std::get<String>(nameVal);

			auto classInfo = std::make_shared<TypeInfo>(className);

			for (Int i = 0; i < fieldCount; ++i)
			{
				if (Value fieldVal = Pop(); std::holds_alternative<String>(fieldVal))
				{
					classInfo->AddField(std::get<String>(fieldVal));
				}
				else
				{
					std::cerr << "Runtime Error: Field name must be a string\n";
					return InterpreterResult::RuntimeError;
				}
			}

			m_types[className.Hash()] = classInfo;
			break;
		}

		case OpCode::MakeArray: {
			Value sizeVal = Pop();
			auto size = static_cast<std::size_t>(std::get<std::int64_t>(sizeVal));

			auto arr = std::make_shared<ArrayInstance>();
			arr->elements.resize(size, std::monostate{}); // Заполняем null-ами

			Push(arr);
			break;
		}

		case OpCode::IndexLoad: {
			Value indexVal = Pop();
			Value arrVal = Pop();

			auto arr = std::get<std::shared_ptr<ArrayInstance>>(arrVal);
			auto idx = static_cast<std::size_t>(std::get<Int>(indexVal));

			if (idx >= arr->elements.size())
			{
				throw std::runtime_error("Array index out of bounds");
			}

			Push(arr->elements[idx]);
			break;
		}

		case OpCode::IndexStore: {
			Value val = Pop();
			Value indexVal = Pop();
			Value arrVal = Pop();

			auto arr = std::get<std::shared_ptr<ArrayInstance>>(arrVal);
			auto idx = static_cast<std::size_t>(std::get<Int>(indexVal));

			if (idx >= arr->elements.size())
			{
				throw std::runtime_error("Array index out of bounds");
			}

			arr->elements[idx] = std::move(val);
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

void VirtualMachine::Push(Value const& value)
{
	m_stack.push_back(value);
}

} // namespace re::rvm