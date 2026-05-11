#include <RVM/Types.hpp>

#include <Core/Utils.hpp>
#include <RVM/VirtualMachine.hpp>
#include <ranges>

#include <utility>

namespace
{

bool IsZero(const double d) { return std::abs(d) < std::numeric_limits<double>::epsilon(); }

} // namespace

namespace re::rvm
{

using namespace utils;

void Coroutine::Trace(VirtualMachine* vm)
{
	vm->MarkObject(caller.Get());
	vm->MarkValue(transferValue);

	for (auto const& val : stack)
	{
		vm->MarkValue(val);
	}
	for (auto const& val : variables)
	{
		vm->MarkValue(val);
	}

	for (auto const& frame : callFrames)
	{
		vm->MarkObject(frame.closure.Get());
	}
}

TypeInfo::TypeInfo(String name)
	: name(std::move(name))
{
	allocator = [](TypeInfoPtr const& t) -> Value {
		return MakeObject<Instance>(t);
	};
}

void TypeInfo::AddField(String const& fieldName)
{
	if (!fieldIndexes.contains(fieldName))
	{
		fieldIndexes[fieldName] = fieldNames.size();
		fieldNames.push_back(fieldName);
	}
}

TypeInfo& TypeInfo::AddNativeMethod(String const& methodName, const std::int8_t argCount, NativeFn function)
{
	auto nativeObj = MakeObject<NativeObject>();
	nativeObj->name = methodName;
	nativeObj->argCount = argCount;
	nativeObj->function = std::move(function);

	methods[methodName] = nativeObj;

	return *this;
}

TypeInfo& TypeInfo::SetAllocator(AllocatorFn alloc)
{
	allocator = std::move(alloc);

	return *this;
}

TypeInfo& TypeInfo::AddNativeGetter(String const& propName, NativeGetterFn function)
{
	getters[propName] = std::move(function);
	return *this;
}

bool TypeInfo::HasAnnotation(const String& annoName) const noexcept
{
	return annotations.contains(annoName);
}

Value TypeInfo::GetAnnotation(const String& annoName) const noexcept
{
	if (const auto it = annotations.find(annoName); it != annotations.end())
	{
		return it->second;
	}

	return Null;
}

bool TypeInfo::HasFieldAnnotation(const String& fieldName, const String& annoName) const noexcept
{
	if (const auto it = fieldAnnotations.find(fieldName); it != fieldAnnotations.end())
	{
		return it->second.contains(annoName);
	}

	return false;
}

Value TypeInfo::GetFieldAnnotation(const String& fieldName, const String& annoName) const noexcept
{
	if (const auto it = fieldAnnotations.find(fieldName); it != fieldAnnotations.end())
	{
		if (const auto annoIt = it->second.find(annoName); annoIt != it->second.end())
		{
			return annoIt->second;
		}
	}

	return Null;
}

bool TypeInfo::HasMethodAnnotation(const String& methodName, const String& annoName) const noexcept
{
	if (const auto it = methodAnnotations.find(methodName); it != methodAnnotations.end())
	{
		return it->second.contains(annoName);
	}

	return false;
}

Value TypeInfo::GetMethodAnnotation(const String& methodName, const String& annoName) const noexcept
{
	if (const auto it = methodAnnotations.find(methodName); it != methodAnnotations.end())
	{
		if (const auto annoIt = it->second.find(annoName); annoIt != it->second.end())
		{
			return annoIt->second;
		}
	}

	return Null;
}

void TypeInfo::Trace(VirtualMachine* vm)
{
	for (const auto& method : methods | std::views::values)
	{
		vm->MarkValue(method);
	}
}

Instance::Instance(TypeInfoPtr const& typeInfo)
	: typeInfo(typeInfo)
{
	fields.resize(typeInfo->fieldNames.size(), Null);
}

void Instance::Trace(VirtualMachine* vm)
{
	vm->MarkObject(typeInfo.Get());
	for (auto const& field : fields)
	{
		vm->MarkValue(field);
	}
}

void ArrayInstance::Trace(VirtualMachine* vm)
{
	vm->MarkObject(typeInfo.Get());
	for (auto const& elem : elements)
	{
		vm->MarkValue(elem);
	}
}

void Upvalue::Trace(VirtualMachine* vm)
{
	vm->MarkValue(value);
}

void Closure::Trace(VirtualMachine* vm)
{
	for (auto const& uv : captured)
	{
		vm->MarkObject(uv.Get());
	}
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
		[](ArrayInstancePtr const& a, ArrayInstancePtr const& b) -> Value {
			auto result = MakeObject<ArrayInstance>();
			result->elements.reserve(a->elements.size() + b->elements.size());
			result->typeInfo = a->typeInfo;

			result->elements.append_range(a->elements);
			result->elements.append_range(b->elements);

			return result;
		},
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
	// clang-format off
	return std::visit(overloaded{
		[](const Int a, const Int b) -> Value {
			if (b == 0) return Null; // integer division by 0

			return a % b;
		},
		[](const Double a, const Double b) -> Value { return std::fmod(a, b); },
		[](const Int a, const Double b)    -> Value { return std::fmod(static_cast<Double>(a), b); },
		[](const Double a, const Int b)    -> Value { return std::fmod(a, static_cast<Double>(b)); },
		[](const auto&, const auto&)       -> Value { return Null; }
	}, lhs, rhs);
	// clang-format on
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

Value OpAnd(Value const& lhs, Value const& rhs)
{
	if (!IsTruthy(lhs))
	{
		return lhs;
	}

	return rhs;
}

Value OpOr(Value const& lhs, Value const& rhs)
{
	if (IsTruthy(lhs))
	{
		return lhs;
	}

	return rhs;
}

} // namespace re::rvm