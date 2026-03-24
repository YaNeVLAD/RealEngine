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

using Null_t = std::monostate;
using Int = std::int64_t;
using Double = std::double_t;

using TypeInfoPtr = Ptr<TypeInfo>;
using InstancePtr = Ptr<Instance>;
using ArrayInstancePtr = Ptr<ArrayInstance>;

using UpvaluePtr = Ptr<Upvalue>;
using ClosurePtr = Ptr<Closure>;
using NativeObjectPtr = Ptr<NativeObject>;

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
	NativeObjectPtr>;

constexpr auto Null = Value{ Null_t{} };

struct TypeInfo
{
	String name;
	std::unordered_map<Hash_t, std::size_t> fieldIndexes;
	std::vector<String> fieldNames;

	std::unordered_map<Hash_t, Value> methods;

	explicit TypeInfo(String name);

	void AddField(String const& fieldName);
};

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
	std::uint8_t argCount;
	NativeFn function;
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

	// ---------------------------------------------------------
	// LOCAL VARIABLES
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

	// Bytecode: [LoadNative, const_index, arg_count]
	// Stack: ... -> ..., [NativeObjectPtr]
	// Description: Reads 1 byte (name index) and 1 byte (arg count). Finds the
	// function in the registry by name and pushes a new NativeObject onto the stack.
	LoadNative,

	// ---------------------------------------------------------
	// ARITHMETIC
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

	// A > B
	Greater,

	// A <= B
	LessEqual,

	// A >= B
	GreaterEqual,

	// A != B
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

	// Bytecode: [CallIndirect]
	// Stack: ..., [function_ptr], [args...] -> ...
	// Description: Similar to Call, but the target function is popped from the stack.
	// And can be any Callable object like NativeObjectPtr or ClosurePtr
	CallIndirect,

	// Bytecode: [CallMethod, const_index(name_hash), uint8(arg_count)]
	// Stack: ..., [InstancePtr], [arg1], ..., [argN] -> ..., [return_value]
	// Description: Reads 1 constant (method name string) and 1 byte (arg count).
	// Pops N arguments, then pops the object instance (self).
	// Looks up the method by name in the object's TypeInfo and invokes it.
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
	// Description: Reads a hash (usually 4-8 bytes). Pops the instance,
	// looks up the field by hash, and pushes the field's value.
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

	// ---------------------------------------------------------
	// TERMINATION
	// ---------------------------------------------------------

	// Bytecode: [Return]
	// Stack: ..., [return_value] -> ..., [return_value]
	// Description: Ends execution of the current function. Pops the return address
	// from the CallStack and restore the instruction pointer. If CallStack is empty, terminates the VM.
	Return = std::numeric_limits<std::uint8_t>::max(),
};

RE_RVM_API Value operator+(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator-(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator*(Value const& lhs, Value const& rhs);
RE_RVM_API Value operator/(Value const& lhs, Value const& rhs);

RE_RVM_API Value OpLess(Value const& lhs, Value const& rhs);
RE_RVM_API Value OpEqual(Value const& lhs, Value const& rhs);

RE_RVM_API bool IsTruthy(Value const& val);

RE_RVM_API std::ostream& operator<<(std::ostream& os, Value const& val);

} // namespace re::rvm