#pragma once

#include <RVM/Export.hpp>

#include <Core/String.hpp>

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <variant>

namespace re::rvm
{

template <typename T>
using Ptr = std::shared_ptr<T>;

struct TypeInfo;
struct Instance;
struct ArrayInstance;

struct Upvalue;
struct Closure;
struct NativeObject;

struct CallFrame;
struct Coroutine;

using Null_t = std::monostate;
using Int = std::int64_t;
using Double = std::double_t;

using TypeInfoPtr = Ptr<TypeInfo>;
using InstancePtr = Ptr<Instance>;
using ArrayInstancePtr = Ptr<ArrayInstance>;

using UpvaluePtr = Ptr<Upvalue>;
using ClosurePtr = Ptr<Closure>;
using NativeObjectPtr = Ptr<NativeObject>;

using CoroutinePtr = Ptr<Coroutine>;

using Value = std::variant<
	Null_t,
	Int,
	Double,
	String,
	TypeInfoPtr,
	InstancePtr,
	ArrayInstancePtr,
	UpvaluePtr,
	ClosurePtr,
	NativeObjectPtr,
	CoroutinePtr>;

constexpr auto Null = Value{ Null_t{} };

struct Instance
{
	TypeInfoPtr typeInfo;
	std::vector<Value> fields;

	explicit Instance(TypeInfoPtr const& typeInfo);
};

struct ArrayInstance
{
	TypeInfoPtr typeInfo;
	std::vector<Value> elements;
};

struct Upvalue
{
	Value value;
};

struct Closure
{
	std::int64_t ipOffset;
	std::vector<UpvaluePtr> captured;
};

using NativeFn = std::function<Value(std::vector<Value> const&)>;

struct NativeObject
{
	String name;
	std::int8_t argCount;
	NativeFn function;
};

enum class CoroutineState : std::uint8_t
{
	Suspended,
	Running,
	Dead,
};

struct CallFrame
{
	const std::uint8_t* returnAddress;
	std::size_t stackBase;
	std::size_t localsBase;

	ClosurePtr closure;
};

struct Coroutine
{
	CoroutineState state = CoroutineState::Suspended;
	const std::uint8_t* ip = nullptr;
	std::size_t currentLocalsBase = 0;

	std::vector<Value> stack;
	std::vector<Value> variables;
	std::vector<CallFrame> callFrames;

	Value transferValue = Null;

	CoroutinePtr caller = nullptr;
};

using AllocatorFn = std::function<Value(TypeInfoPtr const&)>;
using NativeGetterFn = std::function<Value(const Value& obj)>;

struct TypeInfo
{
	String name;
	std::unordered_map<String, std::size_t> fieldIndexes;
	std::vector<String> fieldNames;

	std::unordered_map<String, Value> methods;

	std::unordered_map<String, NativeGetterFn> getters;

	AllocatorFn allocator;

	explicit TypeInfo(String name);

	void AddField(String const& fieldName);

	TypeInfo& AddNativeMethod(String const& methodName, std::int8_t argCount, NativeFn function);

	TypeInfo& SetAllocator(AllocatorFn alloc);

	TypeInfo& AddNativeGetter(String const& propName, NativeGetterFn function);
};

enum class OpCode : std::uint8_t
{
	// ---------------------------------------------------------
	// BASIC OPERATIONS
	// ---------------------------------------------------------

	// Bytecode: [Const, const_index]
	// Stack: ... -> ..., [constant_value]
	// Description: Reads 1 byte (index). Fetches the value from the constant pool
	// at this index and pushes it onto the top of the stack.
	Const,

	// Bytecode: [Pop]
	// Stack: ..., [value] -> ...
	// Description: Pops and discards the top value from the stack.
	// Used for stack cleanup after evaluating expression statements.
	Pop,

	// Bytecode: [Dup]
	// Stack: ..., [value] -> ..., [value], [value]
	// Description: Duplicates the top value on the stack.
	Dup,

	// ---------------------------------------------------------
	// LOCAL & GLOBAL VARIABLES
	// ---------------------------------------------------------

	// Bytecode: [GetLocal, slot_index]
	// Stack: ... -> ..., [variable_value]
	// Description: Reads 1 byte (variable index). Copies the value
	// from the variable array (m_variables[slot_index]) to the top of the stack.
	GetLocal,

	// Bytecode: [SetLocal, slot_index]
	// Stack: ..., [value] -> ...
	// Description: Reads 1 byte (variable index). Pops the top value
	// from the stack and stores it in the variable array (m_variables[slot_index] = value).
	SetLocal,

	// Bytecode: [GetGlobal, hash_index]
	// Stack: ... -> ..., [value]
	// Description: Reads a hash index. Fetches the value associated with this
	// name from the global storage (m_globals) and pushes it onto the stack.
	GetGlobal,

	// Bytecode: [SetGlobal, hash_index]
	// Stack: ..., [value] -> ...
	// Description: Reads a hash index. Pops the top value and stores it
	// in the global storage (m_globals) under the associated name.
	SetGlobal,

	// Bytecode: [IncLocal, slot_index]
	// Stack: ... -> ...
	// Description: Reads 1 byte (variable index). Directly increments the value
	// in the variable array at slot_index without affecting the stack.
	IncLocal,

	// Bytecode: [JmpIfGreaterEqualLocal, slot_i, slot_limit, offset]
	// Stack: ... -> ...
	// Description: Compares values in two local slots (i >= limit). If true,
	// moves the IP by the provided offset.
	JmpIfGreaterEqualLocal,

	// Bytecode: [JmpIfGreaterLocal, slot_i, slot_limit, offset]
	// Stack: ... -> ...
	// Description: Compares values in two local slots (i > limit). If true,
	// moves the IP by the provided offset.
	JmpIfGreaterLocal,

	// ---------------------------------------------------------
	// ARITHMETIC & LOGIC
	// ---------------------------------------------------------

	// Bytecode: [Add]
	// Stack: ..., [a], [b] -> ..., [a + b]
	// Description: Pops 2 values from the stack, adds them, and pushes the result back.
	Add,

	// Bytecode: [Sub]
	// Stack: ..., [a], [b] -> ..., [a - b]
	// Description: Pops 2 values. Subtracts the top value (b) from the lower value (a).
	Sub,

	// Bytecode: [Mul]
	// Stack: ..., [a], [b] -> ..., [a * b]
	// Description: Pops 2 values, multiplies them, and pushes the result back.
	Mul,

	// Bytecode: [Div]
	// Stack: ..., [a], [b] -> ..., [a / b]
	// Description: Pops 2 values. Divides the lower value (a) by the top value (b).
	Div,

	// Bytecode: [Mod]
	// Stack: ..., [a], [b] -> ..., [a % b]
	// Description: Pops 2 values. Performs modulo (a % b) and pushes the result back.
	Mod,

	// Bytecode: [Inc]
	// Stack: ..., [a] -> ..., [a + 1]
	// Description: Pops the top value, increments it by 1, and pushes it back.
	Inc,

	// Bytecode: [Dec]
	// Stack: ..., [a] -> ..., [a - 1]
	// Description: Pops the top value, decrements it by 1, and pushes it back.
	Dec,

	// Bytecode: [BitNot]
	// Stack: ..., [a] -> ..., [~a]
	// Description: Performs bitwise NOT on the top stack value.
	BitNot,

	// ---------------------------------------------------------
	// COMPARISON
	// ---------------------------------------------------------

	// Bytecode: [Less]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A < B, else 0 (false).
	Less,

	// Bytecode: [Equal]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A == B, else 0 (false).
	Equal,

	// Bytecode: [Greater]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A > B, else 0 (false).
	Greater,

	// Bytecode: [LessEqual]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A <= B, else 0 (false).
	LessEqual,

	// Bytecode: [GreaterEqual]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A >= B, else 0 (false).
	GreaterEqual,

	// Bytecode: [NotEqual]
	// Stack: ..., [a], [b] -> ..., [bool]
	// Description: Pops B and A. Pushes 1 (true) if A != B, else 0 (false).
	NotEqual,

	// ---------------------------------------------------------
	// CLOSURE AND UPVALUE
	// ---------------------------------------------------------

	// Bytecode: [MakeClosure, const_index, upvalue_count]
	// Stack: ..., [uv1], ..., [uvN] -> ..., [ClosurePtr]
	// Description: Reads 1 byte (IP offset index) and 1 byte (count). Pops N UpvaluePtrs
	// from the stack to create a new Closure. Pushes the Closure back.
	MakeClosure,

	// Bytecode: [Box]
	// Stack: ..., [value] -> ..., [UpvaluePtr]
	// Description: Pops a Value, wraps it into a heap-allocated Upvalue,
	// and pushes the pointer back onto the stack.
	Box,

	// Bytecode: [Unbox]
	// Stack: ..., [UpvaluePtr] -> ..., [value]
	// Description: Pops an UpvaluePtr, extracts the inner Value, and pushes it onto the stack.
	Unbox,

	// Bytecode: [StoreBox]
	// Stack: ..., [UpvaluePtr], [value] -> ...
	// Description: Pops a Value and an UpvaluePtr. Updates the value stored
	// inside the referenced Upvalue.
	StoreBox,

	// Bytecode: [GetUpvalue, index]
	// Stack: ... -> ..., [value]
	// Description: Reads 1 byte (index). Accesses the Upvalue at the given index
	// in the current closure and pushes its current value onto the stack.
	GetUpvalue,

	// Bytecode: [SetUpvalue, index]
	// Stack: ..., [value] -> ...
	// Description: Reads 1 byte (index). Pops a value and writes it into the
	// Upvalue at the given index in the current closure.
	SetUpvalue,

	// ---------------------------------------------------------
	// CONTROL FLOW
	// ---------------------------------------------------------

	// Bytecode: [JmpIfFalse, offset]
	// Stack: ..., [condition] -> ...
	// Description: Pops the condition. If it is "falsy", moves the IP by the
	// provided offset. Otherwise, continues execution from the next instruction.
	JmpIfFalse,

	// Bytecode: [Jmp, offset]
	// Stack: ... -> ...
	// Description: Unconditionally moves the IP by the provided offset.
	Jmp,

	// Bytecode: [Call, const_index]
	// Stack: ..., [arg1], [arg2] -> ..., [arg1], [arg2] (remain on stack)
	// Description: Reads 1 byte (constant index). The constant stores a byte offset.
	// Saves the current IP (return address) to the CallStack, then moves IP to the offset.
	Call,

	// Bytecode: [Native, const_index, arg_count]
	// Stack: ..., [arg1], ..., [argN] -> ..., [return_value]
	// Description: Reads 2 bytes. const_index contains the function name (String);
	// arg_count is the number of arguments. Pops N values, calls the C++ function,
	// and pushes the return value back onto the stack.
	Native,

	// Bytecode: [LoadNative, const_index, arg_count]
	// Stack: ... -> ..., [NativeObjectPtr]
	// Description: Reads 1 byte (name index) and 1 byte (arg count). Finds the
	// function in the registry by name and pushes a new NativeObject onto the stack.
	LoadNative,

	// Bytecode: [CallIndirect]
	// Stack: ..., [function_ptr], [args...] -> ...
	// Description: Similar to Call, but the target function is popped from the stack.
	// Can be any Callable object like NativeObjectPtr or ClosurePtr.
	CallIndirect,

	// Bytecode: [CallMethod, const_index, arg_count]
	// Stack: ..., [InstancePtr], [arg1], ..., [argN] -> ..., [return_value]
	// Description: Reads 1 constant (method name hash) and 1 byte (arg count).
	// Pops N arguments, then pops the object instance. Looks up and invokes the method.
	CallMethod,

	// ---------------------------------------------------------
	// USER DEFINED TYPES AND REFLECTION
	// ---------------------------------------------------------

	// Bytecode: [New]
	// Stack: ..., [TypeInfoPtr] -> ..., [InstancePtr]
	// Description: Pops a TypeInfo, creates a new Instance of that type
	// with default-initialized fields, and pushes it onto the stack.
	New,

	// Bytecode: [GetProperty, name_hash]
	// Stack: ..., [InstancePtr] -> ..., [value]
	// Description: Reads a hash. Pops the instance, looks up the field
	// by hash, and pushes the field's value.
	GetProperty,

	// Bytecode: [SetProperty, name_hash]
	// Stack: ..., [InstancePtr], [value] -> ...
	// Description: Reads a hash. Pops the value and the instance, then
	// assigns the value to the specified field of the instance.
	SetProperty,

	// Bytecode: [TypeOf]
	// Stack: ..., [value] -> ..., [TypeInfoPtr]
	// Description: Pops a value and pushes its corresponding TypeInfo object.
	TypeOf,

	// Bytecode: [DefType, field_count]
	// Stack: ..., [field_nameN], ..., [type_name] -> ...
	// Description: Reads 1 byte (fields count). Pops field names and the type name
	// to register a new TypeInfo structure in the VM.
	DefType,

	// Bytecode: [BindMethod, class_const, method_const]
	// Stack: ..., [Closure] -> ...
	// Description: Reads two constant indices. Pops a closure and binds it
	// as a method to the specified class.
	BindMethod,

	// ---------------------------------------------------------
	// ARRAYS
	// ---------------------------------------------------------

	// Bytecode: [PackArray, count]
	// Stack: ..., [val1], ..., [valN] -> ..., [ArrayObjectPtr]
	// Description: Reads 1 byte (count). Pops N elements from the stack,
	// packs them into a new Array object, and pushes the array back.
	PackArray,

	// ---------------------------------------------------------
	// COROUTINES & ASYNC
	// ---------------------------------------------------------

	// Bytecode: [CoroutineMake]
	// Stack: ..., [ClosurePtr] -> ..., [CoroutinePtr]
	// Description: Pops a closure, allocates a new Coroutine in the heap,
	// initializes its first CallFrame with the closure, and pushes the CoroutinePtr.
	CoroutineMake,

	// Bytecode: [CoroutineResume]
	// Stack: ..., [CoroutinePtr], [arg] -> ..., [yielded_value]
	// Description: Resumes a suspended coroutine, passing 'arg' to it.
	// The VM suspends the current frame, switches to the coroutine's context,
	// and continues execution until it yields or returns.
	CoroutineResume,

	// Bytecode: [CoroutineYield]
	// Stack: ..., [value] -> ...
	// Description: Pauses the current coroutine, saving its IP. Passes 'value'
	// back to the caller's CoroutineResume instruction.
	CoroutineYield,

	// Bytecode: [CoroutineAwait]
	// Stack: ..., [Future/Task] -> ..., [result]
	// Description: Pauses the coroutine, yielding control back to the Event Loop
	// until the awaited Task is resolved.
	CoroutineAwait,

	// Bytecode: [CoroutineLaunch]
	// Stack: ..., [ClosurePtr] -> ...
	// Description: Pops a closure, allocates a new Coroutine in the heap,
	// initializes its first CallFrame, and immediately schedules it into the
	// VM's internal microtask queue. The coroutine will be executed asynchronously.
	// Unlike CoroutineMake, it does not push the CoroutinePtr back onto the stack.
	CoroutineLaunch,

	// ---------------------------------------------------------
	// TERMINATION
	// ---------------------------------------------------------

	// Bytecode: [Return]
	// Stack: ..., [return_value] -> ..., [return_value]
	// Description: Ends execution of the current function. Pops the return address
	// from the CallStack and restores the IP. If CallStack is empty, terminates the VM.
	Return = std::numeric_limits<std::uint8_t>::max(),
};

RE_RVM_API Value operator+(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator-(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator*(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator/(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator%(Value const& lhs, Value const& rhs);

RE_RVM_API Value OpLess(Value const& lhs, Value const& rhs);
RE_RVM_API Value OpEqual(Value const& lhs, Value const& rhs);

RE_RVM_API bool IsTruthy(Value const& val);

template <typename TChar>
std::basic_ostream<TChar>& operator<<(std::basic_ostream<TChar>& os, Value const& val);

} // namespace re::rvm

#include <RVM/Types.inl>