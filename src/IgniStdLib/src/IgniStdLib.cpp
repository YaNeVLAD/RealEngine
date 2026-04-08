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
		const auto arr = std::get<ArrayInstancePtr>(args[0]);

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

	auto arrayType = vm->GetTypeByName("Array");
	if (arrayType)
	{
		InitArrayMethods(arrayType);
	}

	InitMathModule(vm);

	vm->RegisterNative("print", [](const std::vector<Value>& args) -> Value {
		std::cout << ">>> [SCRIPT]: ";
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
		std::cout << "\n";

		return Null;
	});

	vm->RegisterNative("len", [](const std::vector<Value>& args) -> Value {
		if (const auto arr = std::get_if<ArrayInstancePtr>(&args[0]))
		{
			return static_cast<Int>((*arr)->elements.size());
		}

		return Int{};
	});
}