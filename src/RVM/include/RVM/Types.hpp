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
	Const,

	Pop,
	GetLocal,
	SetLocal,

	Add,
	Sub,
	Mul,
	Div,

	Call,
	Native,

	Return = std::numeric_limits<std::uint8_t>::max(),
};

Value operator+(const Value& lhs, const Value& rhs);
Value operator-(const Value& lhs, const Value& rhs);
Value operator*(const Value& lhs, const Value& rhs);
Value operator/(const Value& lhs, const Value& rhs);

std::ostream& operator<<(std::ostream& os, const Value& val);

} // namespace re::rvm