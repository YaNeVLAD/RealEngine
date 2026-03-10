#include <RVM/Types.hpp>

#include <Core/Utils.hpp>

namespace
{

bool IsZero(const double d) { return std::abs(d) < 1e-9; }

} // namespace

namespace re::rvm
{

using namespace utils;

Value operator+(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](Int a, Int b)       				 -> Value { return a + b; },
		[](Double a, Double b) 				 -> Value { return a + b; },
		[](Int a, Double b)    				 -> Value { return static_cast<Double>(a) + b; },
		[](Double a, Int b)    				 -> Value { return a + static_cast<Double>(b); },
		[](String const& a, String const& b) -> Value { return a + b; },
		[](auto, auto)						 -> Value { return std::monostate{}; }
	}, lhs, rhs);
	// clang-format on
}

Value operator-(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](Int a, Int b)       -> Value { return a - b; },
		[](Double a, Double b) -> Value { return a - b; },
		[](Int a, Double b)    -> Value { return static_cast<Double>(a) - b; },
		[](Double a, Int b)    -> Value { return a - static_cast<Double>(b); },
		[](auto, auto)         -> Value { return std::monostate{}; }
	}, lhs, rhs);
	// clang-format on
}

Value operator*(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](Int a, Int b)       -> Value { return a * b; },
		[](Double a, Double b) -> Value { return a * b; },
		[](Int a, Double b)    -> Value { return static_cast<Double>(a) * b; },
		[](Double a, Int b)    -> Value { return a * static_cast<Double>(b); },
		[](auto, auto)         -> Value { return std::monostate{}; }
	}, lhs, rhs);
	// clang-format on
}

Value operator/(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](Int a, Int b) -> Value {
			if (b == 0) return Value(std::monostate{}); // integer division by 0

			return static_cast<Double>(a) / static_cast<Double>(b);
		},
		[](Double a, Double b) -> Value { return a / b; },
		[](Int a, Double b)    -> Value { return static_cast<Double>(a) / b; },
		[](Double a, Int b)    -> Value { return a / static_cast<Double>(b); },
		[](auto, auto)         -> Value { return std::monostate{}; }
	}, lhs, rhs);
	// clang-format on
}

std::ostream& operator<<(std::ostream& os, const Value& val)
{
	// clang-format off
	std::visit(overloaded{
		[&os](std::monostate){ os << "null"; },
		[&os](const Int v){ os << v; },
		[&os](const Double v){ os << v; },
		[&os](String const& str){ os << str.ToString(); },
	}, val);
	return os;
	// clang-format on
}

} // namespace re::rvm