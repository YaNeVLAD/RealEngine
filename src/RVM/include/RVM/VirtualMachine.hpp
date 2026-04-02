#pragma once

#include <RVM/Export.hpp>

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

struct CallFrame
{
	const std::uint8_t* returnAddress;
	std::size_t stackBase;
	std::size_t localsBase;

	ClosurePtr closure;
};

class RE_RVM_API VirtualMachine
{
public:
	VirtualMachine();

	InterpreterResult Interpret(Chunk const& chunk);

	void RegisterNative(String const& name, NativeFn fn);

	void RegisterType(TypeInfoPtr typeInfo);

	void RegisterGlobal(const String& name, Value value);

	[[nodiscard]] TypeInfoPtr GetType(Value const& value) const;

	[[nodiscard]] TypeInfoPtr GetTypeByName(String const& name) const;

private:
	InterpreterResult Run();

	Value Pop();
	Value const& Peek();
	void Push(Value const& value);

private:
	template <typename T>
	using HashMap = std::unordered_map<String, T>;

	void InitBuiltinTypes();

	TypeInfoPtr m_typeInt;
	TypeInfoPtr m_typeDouble;
	TypeInfoPtr m_typeString;
	TypeInfoPtr m_typeArray;
	TypeInfoPtr m_typeNull;

	const Chunk* m_chunk = nullptr;
	const std::uint8_t* m_ip = nullptr;

	std::vector<Value> m_stack;
	std::vector<Value> m_variables;

	std::vector<CallFrame> m_callStack;
	HashMap<NativeFn> m_natives;

	HashMap<TypeInfoPtr> m_types;

	std::unordered_map<String, Value> m_globals;

	std::size_t m_currentLocalsBase = 0;
};

} // namespace re::rvm