#include <IgniStdLib/Export.hpp>

#include <Core/String.hpp>
#include <RVM/VirtualMachine.hpp>

#include <cmath>
#include <iostream>

namespace
{

void InitArrayMethods(const re::rvm::TypeInfoPtr& arrayType)
{
	using namespace re::rvm;
	arrayType->SetAllocator([](const TypeInfoPtr& type) {
		auto arr = std::make_shared<ArrayInstance>();
		arr->typeInfo = type;

		return arr;
	});

	arrayType->AddNativeMethod("init", -1, [](const std::vector<Value>& args) -> Value {
		auto arr = std::get<ArrayInstancePtr>(args[0]);

		if (args.size() == 2)
		{
			const auto size = std::get<Int>(args[1]);
			arr->elements.resize(size);
		}

		return arr;
	});

	arrayType->AddNativeMethod("set", 2, [](const std::vector<Value>& args) -> Value {
		const auto arr = std::get<ArrayInstancePtr>(args[0]);
		const auto index = std::get<Int>(args[1]);

		arr->elements[index] = args[2];

		return Null;
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
		InitArrayMethods(arrayType);
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

	vm->RegisterNative("arrayOf", [](const std::vector<Value>& args) -> Value {
		if (!std::holds_alternative<ArrayInstancePtr>(args[0]))
		{
			return Null;
		}

		const auto paramArr = std::get<ArrayInstancePtr>(args[0]);
		auto returnArr = std::make_shared<ArrayInstance>();

		returnArr->elements = paramArr->elements;
		returnArr->typeInfo = paramArr->typeInfo;

		return returnArr;
	});
}