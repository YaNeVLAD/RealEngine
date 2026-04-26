#include <IgniStdLib/Export.hpp>

#include <Core/String.hpp>
#include <RVM/VirtualMachine.hpp>

#include <iostream>

namespace
{

void InitArrayMethods(re::rvm::VirtualMachine* vm, const re::rvm::TypeInfoPtr& arrayType)
{
	using namespace re::rvm;

	const auto mutableArrayType = std::make_shared<TypeInfo>("MutableArray");
	vm->RegisterType(mutableArrayType);

	auto allocator = [](const TypeInfoPtr& type) {
		auto arr = std::make_shared<ArrayInstance>();
		arr->typeInfo = type;
		return arr;
	};
	arrayType->SetAllocator(allocator);
	mutableArrayType->SetAllocator(allocator);

	arrayType->AddNativeMethod("init", -1, [](const std::vector<Value>& args) -> Value {
		auto arr = std::get<ArrayInstancePtr>(args[0]);

		if (args.size() == 2)
		{
			if (auto* sizePtr = std::get_if<Int>(&args[1]))
			{
				arr->elements.resize(*sizePtr);
			}
			else if (auto* otherArrPtr = std::get_if<ArrayInstancePtr>(&args[1]))
			{
				arr->elements = (*otherArrPtr)->elements;
			}
		}

		return arr;
	});

	arrayType->AddNativeMethod("get", 1, [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		const auto index = std::get<Int>(args[1]);

		return arr->elements[index];
	});

	arrayType->AddNativeGetter("size", [](const Value& obj) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(obj);

		return static_cast<Int>(arr->elements.size());
	});

	arrayType->AddNativeGetter("length", [](const Value& obj) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(obj);

		return static_cast<Int>(arr->elements.size());
	});

	mutableArrayType->methods = arrayType->methods;
	mutableArrayType->getters = arrayType->getters;

	mutableArrayType->AddNativeMethod("set", 2, [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		const auto index = std::get<Int>(args[1]);

		arr->elements[index] = args[2];

		return Null;
	});

	mutableArrayType->AddNativeMethod("push", 1, [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		arr->elements.push_back(args[1]);

		return Null;
	});

	mutableArrayType->AddNativeMethod("toArray", 0, [arrayType](const std::vector<Value>& args) -> Value {
		const auto mutArr = std::get<ArrayInstancePtr>(args[0]);

		auto readOnlyArr = std::make_shared<ArrayInstance>();

		readOnlyArr->elements = mutArr->elements;

		readOnlyArr->typeInfo = arrayType;

		return readOnlyArr;
	});
}

void InitStringMethods(const re::rvm::TypeInfoPtr& arrayType)
{
	using namespace re::rvm;

	arrayType->AddNativeGetter("size", [](const Value& obj) -> Value {
		const auto& str = std::get<re::String>(obj);

		return static_cast<Int>(str.Size());
	});

	arrayType->AddNativeGetter("length", [](const Value& obj) -> Value {
		const auto& str = std::get<re::String>(obj);

		return static_cast<Int>(str.Length());
	});

	arrayType->AddNativeMethod("get", 1, [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<re::String>(args[0]);
		const auto index = std::get<Int>(args[1]);

		return re::String(arr[index]);
	});
}

void InitMathModule(re::rvm::VirtualMachine* vm)
{
	using namespace re::rvm;

	auto mathType = std::make_shared<TypeInfo>("Math");

	mathType
		->AddNativeMethod("sqrt", 1, [](const std::vector<Value>& args) -> Value {
			return std::sqrt(std::get<Double>(args[1]));
		})
		.AddNativeMethod("sin", 1, [](const std::vector<Value>& args) -> Value {
			return std::sin(std::get<Double>(args[1]));
		})
		.AddNativeMethod("cos", 1, [](const std::vector<Value>& args) -> Value {
			return std::cos(std::get<Double>(args[1]));
		})
		.AddNativeMethod("pow", 2, [](const std::vector<Value>& args) -> Value {
			return std::pow(std::get<Double>(args[1]), std::get<Double>(args[2]));
		});

	auto mathInstance = std::make_shared<Instance>(mathType);

	vm->RegisterGlobal("math", mathInstance);
}

} // namespace

IGNI_STD_API void IgniPluginInit(re::rvm::VirtualMachine* vm)
{
	using namespace re::rvm;

	if (const auto arrayType = vm->GetTypeByName("Array"))
	{
		InitArrayMethods(vm, arrayType);
	}

	if (const auto arrayType = vm->GetTypeByName("String"))
	{
		InitStringMethods(arrayType);
	}

	InitMathModule(vm);

	vm->RegisterNative("print", [](const std::vector<Value>& args) -> Value {
		if (!args.empty() && std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			for (const auto arr = std::get<ArrayInstancePtr>(args[0]); const auto& arg : arr->elements)
			{
				std::cout << arg;
			}
		}
		else
		{
			for (const auto& arg : args)
			{
				std::cout << arg;
			}
		}

		return Null;
	});

	vm->RegisterNative("println", [](const std::vector<Value>& args) -> Value {
		if (!args.empty() && std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			for (const auto arr = std::get<ArrayInstancePtr>(args[0]); const auto& arg : arr->elements)
			{
				std::cout << arg;
			}
		}
		else
		{
			for (const auto& arg : args)
			{
				std::cout << arg;
			}
		}
		std::cout << std::endl;

		return Null;
	});

	vm->RegisterNative("arrayOf", [vm](const std::vector<Value>& args) -> Value {
		if (!std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			return Null;
		}

		const auto paramArr = std::get<ArrayInstancePtr>(args[0]);
		auto returnArr = std::make_shared<ArrayInstance>();

		returnArr->elements = paramArr->elements;
		returnArr->typeInfo = vm->GetTypeByName("Array");

		return returnArr;
	});

	vm->RegisterNative("arrayOf", [vm](const std::vector<Value>& args) -> Value {
		if (!std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			return Null;
		}

		const auto paramArr = std::get<ArrayInstancePtr>(args[0]);
		auto returnArr = std::make_shared<ArrayInstance>();

		returnArr->elements = paramArr->elements;
		returnArr->typeInfo = vm->GetTypeByName("MutableArray");

		return returnArr;
	});

	vm->RegisterNative("delay", [vm](const std::vector<Value>& args) -> Value {
		if (args.empty() || !std::holds_alternative<Int>(args[0]))
		{
			return Null;
		}

		const std::uint64_t ms = std::get<Int>(args[0]);

		vm->RequestDelay(ms);

		return Null;
	});
}