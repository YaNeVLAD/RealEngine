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

void InitStringMethods(const re::rvm::TypeInfoPtr& stringType)
{
	using namespace re::rvm;

	auto getString = [](const Value& value) -> std::string {
		return std::get<re::String>(value).ToString();
	};

	stringType->AddNativeGetter("size", [](const Value& obj) -> Value {
		const auto& str = std::get<re::String>(obj);

		return static_cast<Int>(str.Size());
	});

	stringType->AddNativeGetter("length", [](const Value& obj) -> Value {
		const auto& str = std::get<re::String>(obj);

		return static_cast<Int>(str.Length());
	});

	stringType->AddNativeMethod("get", 1, [](const std::vector<Value>& args) -> Value {
		const auto str = std::get<re::String>(args[0]);
		const auto index = std::get<Int>(args[1]);

		if (index < 0 || index >= str.Size())
		{
			return re::String{};
		}

		return re::String(str[index]);
	});

	stringType->AddNativeMethod("substring", 2, [getString](const std::vector<Value>& args) -> Value {
		const auto& s = getString(args[0]);

		const int start = static_cast<int>(std::get<Int>(args[1]));
		const int end = static_cast<int>(std::get<Int>(args[2]));

		if (start < 0 || end > s.size() || start > end)
		{
			return re::String{};
		}

		return s.substr(start, end - start);
	});

	stringType->AddNativeMethod("indexOf", 1, [getString](const std::vector<Value>& args) -> Value {
		const auto s = getString(args[0]);
		const auto sub = getString(args[1]);

		const auto pos = s.find(sub);

		return pos == re::String::NPos ? static_cast<Int>(-1) : static_cast<Int>(pos);
	});

	stringType->AddNativeMethod("trim", 0, [getString](const std::vector<Value>& args) -> Value {
		auto s = getString(args[0]);
		s.erase(0, s.find_first_not_of(" \t\n\r"));
		s.erase(s.find_last_not_of(" \t\n\r") + 1);

		return s;
	});

	stringType->AddNativeMethod("toInt", 0, [getString](const std::vector<Value>& args) -> Value {
		try
		{
			return std::stoll(getString(args[0]));
		}
		catch (...)
		{
			return 0;
		}
	});

	stringType->AddNativeMethod("toDouble", 0, [getString](const std::vector<Value>& args) -> Value {
		try
		{
			return std::stod(getString(args[0]));
		}
		catch (...)
		{
			return 0;
		}
	});

	stringType->AddNativeMethod("startsWith", 1, [getString](const std::vector<Value>& args) -> Value {
		const auto s = getString(args[0]);
		const auto prefix = getString(args[1]);

		return s.compare(0, prefix.size(), prefix) == 0;
	});

	stringType->AddNativeMethod("endsWith", 1, [getString](const std::vector<Value>& args) -> Value {
		const auto s = getString(args[0]);
		const auto suffix = getString(args[1]);
		if (s.size() < suffix.size())
		{
			return 0;
		}

		return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
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

	vm->RegisterNative("mutableArrayOf", [vm](const std::vector<Value>& args) -> Value {
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