#pragma once

#include <RVM/Export.hpp>

#include <RVM/Chunk.hpp>
#include <RVM/Types.hpp>

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace re::rvm
{

enum class InterpreterResult : std::uint8_t
{
	Success = 0,
	RuntimeError,
	Suspended,
};

class RE_RVM_API VirtualMachine
{
public:
	using DelayHandler = std::function<void(CoroutinePtr, std::uint64_t)>;

	VirtualMachine();

	InterpreterResult Interpret(Chunk const& chunk);

	InterpreterResult Resume();

	void RegisterNative(String const& name, NativeFn fn);

	void RegisterType(TypeInfoPtr typeInfo);

	void RegisterGlobal(const String& name, Value value);

	InterpreterResult ResumeCoroutine(CoroutinePtr const& coro, Value const& arg);

	[[nodiscard]] TypeInfoPtr GetType(Value const& value) const;

	[[nodiscard]] TypeInfoPtr GetTypeByName(String const& name) const;

	[[nodiscard]] CoroutinePtr GetActiveCoroutine() const;

	void EnqueueMacrotask(CoroutinePtr const& coro, Value const& arg = Null);

	void SetDelayHandler(DelayHandler handler);

	void RequestDelay(std::uint64_t ms) const;

private:
	InterpreterResult Run();

	Value Pop();
	Value const& Peek();
	void Push(Value const& value);

private:
	template <typename T>
	using HashMap = std::unordered_map<String, T>;

	void InitBuiltinTypes();

	void SaveContext();
	void LoadContext();

	bool SwitchToNextMicrotask();

private:
	CoroutinePtr m_activeCoro;

	std::queue<CoroutinePtr> m_microtasks;

	DelayHandler m_delayHandler;

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