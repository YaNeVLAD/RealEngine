#pragma once

#include <cstdint>

namespace re::scripting
{

struct ManagedInvokeResult
{
	std::int32_t status; // 0 = success, <0 = error
	std::int32_t returnBytes;
};

struct DotNetInterop
{
	using LoadUserAssemblyFn = int (*)(const char*);
	using CreateInstanceFn = void* (*)(const char*, std::uint64_t);
	using InvokeOnCreateFn = void (*)(void*);
	using InvokeOnUpdateFn = void (*)(void*, float);
	using FreeInstanceFn = void (*)(void*);
	using OnDestroyFn = void (*)(void*);
	using CheckClassExistsFn = int (*)(const char*);
	using InvokeMethodByNameFn = ManagedInvokeResult (*)(void*, const char* methodName, const void* args, std::uint32_t argsSize, void* outReturn, std::uint32_t outCapacity);

	static LoadUserAssemblyFn LoadUserAssembly;
	static CreateInstanceFn CreateInstance;
	static InvokeOnCreateFn InvokeOnCreate;
	static InvokeOnUpdateFn InvokeOnUpdate;
	static FreeInstanceFn FreeInstance;
	static OnDestroyFn OnDestroy;
	static CheckClassExistsFn CheckClassExists;
	static InvokeMethodByNameFn InvokeMethodByName;
};

} // namespace re::scripting