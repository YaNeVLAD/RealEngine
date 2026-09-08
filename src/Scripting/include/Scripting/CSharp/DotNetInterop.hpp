#pragma once

#include <cstdint>

namespace re::scripting
{

struct DotNetInterop
{
	using LoadUserAssemblyFn = int (*)(const char*);
	using CreateInstanceFn = void* (*)(const char*, std::uint64_t);
	using InvokeOnCreateFn = void (*)(void*);
	using InvokeOnUpdateFn = void (*)(void*, float);
	using FreeInstanceFn = void (*)(void*);

	using CheckClassExistsFn = int (*)(const char*);
	using InvokeMethodByNameFn = void (*)(void*, const char*);

	static LoadUserAssemblyFn LoadUserAssembly;
	static CreateInstanceFn CreateInstance;
	static InvokeOnCreateFn InvokeOnCreate;
	static InvokeOnUpdateFn InvokeOnUpdate;
	static FreeInstanceFn FreeInstance;
	static CheckClassExistsFn CheckClassExists;
	static InvokeMethodByNameFn InvokeMethodByName;
};

} // namespace re::scripting