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

	// [LoadNative, const_index(name), uint8(argCount)]
	// Находит нативную функцию в реестре, создает NativeObject и кладет на стек.
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

	// Stack: ..., [a] -> ..., [a + 1]
	Inc,

	// Stack: ..., [a] -> ..., [a - 1]
	Dec,

	// Stack: ..., [a] -> ..., [~a]
	BitNot,

	// ---------------------------------------------------------
	// COMPARISON
	// ---------------------------------------------------------

	// Pop B, Pop A -> Push (A < B ? 1 : 0)
	Less,

	// Pop B, Pop A -> Push (A == B ? 1 : 0)
	Equal,

	// ---------------------------------------------------------
	// CLOSURE AND UPVALUE
	// ---------------------------------------------------------

	// [MakeClosure, const_index(offset), uint8(upvalue_count)]
	// Removes the upvalue_count of cells (Upvalue) from the stack, creates a Closure and puts it on the stack.
	MakeClosure,

	// [Box]
	// Removes the Value, packs it into an Upvalue, puts it on the stack.
	Box,

	// [Unbox]
	// Removes the Upvalue, removes the Value from it, puts it on the stack.
	Unbox,

	// [StoreBox]
	// Removes the Value, then removes the Upvalue. Writes the Value inside the Upvalue.
	StoreBox,

	// [GetUpvalue, uint8(index)]
	// Takes the value from the current closure and puts it on the stack.
	GetUpvalue,

	// [SetUpvalue, uint8(index)]
	// Removes Value and writes it to the captured closure variable.
	SetUpvalue,

	// ---------------------------------------------------------
	// CONTROL FLOW
	// ---------------------------------------------------------

	JmpIfFalse,

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

	CallIndirect,

	// ---------------------------------------------------------
	// USER DEFINED TYPES AND REFLECTION
	// ---------------------------------------------------------

	// Stack: ..., [Instance] -> ..., [FieldValue]
	New,

	// Stack: ..., [Instance] -> ..., [FieldValue]
	GetProperty,

	// Stack: ..., [Instance], [Value] -> ...
	SetProperty,

	// Stack: ..., [Instance] -> ..., [ClassInfo]
	TypeOf,

	// Stack: ..., [Field1], [Field2], [ClassName], [FieldCount] -> ...
	DefType,

	// ---------------------------------------------------------
	// ARRAY
	// ---------------------------------------------------------

	// Stack: [size] -> [Array]
	MakeArray,

	// Stack: [Array], [index] -> [Value]
	IndexLoad,

	// Stack: [Array], [index], [Value] -> ...
	IndexStore,

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