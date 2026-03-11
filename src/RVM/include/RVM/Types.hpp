#pragma once

#include <Core/String.hpp>

#include <cstdint>
#include <limits>
#include <ostream>
#include <variant>

namespace re::rvm
{

using Null = std::monostate;
using Int = std::int64_t;
using Double = std::double_t;

using Value = std::variant<Null, Int, Double, String>;

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

	// ---------------------------------------------------------
	// CONTROL FLOW (FUNCTION CALLS)
	// ---------------------------------------------------------

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

	// ---------------------------------------------------------
	// TERMINATION
	// ---------------------------------------------------------

	// Bytecode: [Return]
	// Stack: ..., [return_value] -> ..., [return_value]
	// Description: Ends execution of the current function. Pops the return address
	// from the CallStack and restore the instruction pointer. If CallStack is empty, terminates the VM.
	Return = std::numeric_limits<std::uint8_t>::max(),
};

Value operator+(const Value& lhs, const Value& rhs);
Value operator-(const Value& lhs, const Value& rhs);
Value operator*(const Value& lhs, const Value& rhs);
Value operator/(const Value& lhs, const Value& rhs);

std::ostream& operator<<(std::ostream& os, const Value& val);

} // namespace re::rvm