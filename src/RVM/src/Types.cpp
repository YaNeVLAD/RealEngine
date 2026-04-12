#include <RVM/Types.hpp>

#include <Core/Utils.hpp>

#include <utility>

namespace
{

bool IsZero(const double d) { return std::abs(d) < std::numeric_limits<double>::epsilon(); }

} // namespace

namespace re::rvm
{

using namespace utils;

TypeInfo::TypeInfo(String name)
	: name(std::move(name))
{
	allocator = [](TypeInfoPtr const& t) -> Value {
		return std::make_shared<Instance>(t);
	};
}

void TypeInfo::AddField(String const& fieldName)
{
	if (!fieldIndexes.contains(fieldName.Hash()))
	{
		fieldIndexes[fieldName.Hash()] = fieldNames.size();
		fieldNames.push_back(fieldName);
	}
}

TypeInfo& TypeInfo::AddNativeMethod(String const& methodName, const std::int8_t argCount, NativeFn function)
{
	auto nativeObj = std::make_shared<NativeObject>();
	nativeObj->name = methodName;
	nativeObj->argCount = argCount;
	nativeObj->function = std::move(function);

	methods[methodName.Hash()] = nativeObj;

	return *this;
}

TypeInfo& TypeInfo::SetAllocator(AllocatorFn alloc)
{
	allocator = std::move(alloc);

	return *this;
}

TypeInfo& TypeInfo::AddNativeGetter(String const& propName, NativeGetterFn function)
{
	getters[propName.Hash()] = std::move(function);
	return *this;
}

Instance::Instance(TypeInfoPtr const& typeInfo)
	: typeInfo(typeInfo)
{
	fields.resize(typeInfo->fieldNames.size(), Null);
}

Value operator+(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](const Int a, const Int b)       	 -> Value { return a + b; },
		[](const Double a, const Double b) 	 -> Value { return a + b; },
		[](const Int a, const Double b)    	 -> Value { return static_cast<Double>(a) + b; },
		[](const Double a, const Int b)    	 -> Value { return a + static_cast<Double>(b); },
		[](String const& a, String const& b) -> Value { return a + b; },
		[](String const& a, const Int b)     -> Value { return a + std::to_string(b); },
		[](String const& a, const Double b)  -> Value { return a + std::to_string(b); },
		[](const auto&, const auto&)		 -> Value { return Null; }
	}, lhs, rhs);
	// clang-format on
}

Value operator-(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](const Int a, const Int b)       -> Value { return a - b; },
		[](const Double a, const Double b) -> Value { return a - b; },
		[](const Int a, const Double b)    -> Value { return static_cast<Double>(a) - b; },
		[](const Double a, const Int b)    -> Value { return a - static_cast<Double>(b); },
		[](const auto&, const auto&)       -> Value { return Null; }
	}, lhs, rhs);
	// clang-format on
}

Value operator*(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](const Int a, const Int b)       -> Value { return a * b; },
		[](const Double a, const Double b) -> Value { return a * b; },
		[](const Int a, const Double b)    -> Value { return static_cast<Double>(a) * b; },
		[](const Double a, const Int b)    -> Value { return a * static_cast<Double>(b); },
		[](const auto&, const auto&)       -> Value { return Null; }
	}, lhs, rhs);
	// clang-format on
}

Value operator/(const Value& lhs, const Value& rhs)
{
	// clang-format off
	return std::visit(overloaded{
		[](const Int a, const Int b) -> Value {
			if (b == 0) return Null; // integer division by 0

			return static_cast<Double>(a) / static_cast<Double>(b);
		},
		[](const Double a, const Double b) -> Value { return a / b; },
		[](const Int a, const Double b)    -> Value { return static_cast<Double>(a) / b; },
		[](const Double a, const Int b)    -> Value { return a / static_cast<Double>(b); },
		[](const auto&, const auto&)       -> Value { return Null; }
	}, lhs, rhs);
	// clang-format on
}

Value operator%(Value const& lhs, Value const& rhs)
{
	Value lhs1(lhs);
	return lhs1 %= rhs;
}

Value OpLess(Value const& lhs, Value const& rhs)
{
	return std::visit(overloaded{
						  [](const Int a, const Int b) -> Value { return static_cast<Int>(a < b ? 1 : 0); },
						  [](const Double a, const Double b) -> Value { return static_cast<Int>(a < b ? 1 : 0); },
						  [](const Int a, const Double b) -> Value { return static_cast<Int>(static_cast<Double>(a) < b ? 1 : 0); },
						  [](const Double a, const Int b) -> Value { return static_cast<Int>(a < static_cast<Double>(b) ? 1 : 0); },
						  [](String const& a, String const& b) -> Value { return static_cast<Int>(a.ToString() < b.ToString() ? 1 : 0); },
						  [](const auto&, const auto&) -> Value { return Null; } // Ошибка типов
					  },
		lhs, rhs);
}

Value OpEqual(Value const& lhs, Value const& rhs)
{
	return std::visit(overloaded{
						  [](Null_t, Null_t) -> Value { return static_cast<Int>(1); },
						  [](const Int a, const Int b) -> Value { return static_cast<Int>(a == b ? 1 : 0); },
						  [](const Double a, const Double b) -> Value { return static_cast<Int>(a == b ? 1 : 0); },
						  [](const Int a, const Double b) -> Value { return static_cast<Int>(static_cast<Double>(a) == b ? 1 : 0); },
						  [](const Double a, const Int b) -> Value { return static_cast<Int>(a == static_cast<Double>(b) ? 1 : 0); },
						  [](String const& a, String const& b) -> Value { return static_cast<Int>(a.ToString() == b.ToString() ? 1 : 0); },
						  [](TypeInfoPtr const& a, TypeInfoPtr const& b) -> Value { return static_cast<Int>(a == b ? 1 : 0); },
						  [](const auto&, const auto&) -> Value { return static_cast<Int>(0); },
					  },
		lhs, rhs);
}

bool IsTruthy(Value const& val)
{
	return std::visit(overloaded{
						  [](Null_t) { return false; },
						  [](const Int v) { return v != 0; },
						  [](const Double v) { return !IsZero(v); },
						  [](String const& str) { return !str.Empty(); },
						  [](ArrayInstancePtr const& ptr) { return ptr && !ptr->elements.empty(); },
						  []<typename T>(const Ptr<T>& ptr) { return ptr == nullptr; },
					  },
		val);
}

} // namespace re::rvm