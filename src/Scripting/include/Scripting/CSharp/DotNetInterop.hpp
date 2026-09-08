#pragma once

#include <cstdint>

namespace re::scripting
{

struct EngineApiPointers
{
	void (*NativeLog)(const char* message);
};

struct DotNetInterop
{
	using LoadUserAssemblyFn = int (*)(const char*);
	using CreateInstanceFn = void* (*)(const char*, std::uint64_t);
	using InvokeOnCreateFn = void (*)(void*);
	using InvokeOnUpdateFn = void (*)(void*, float);
	using FreeInstanceFn = void (*)(void*);

	static LoadUserAssemblyFn LoadUserAssembly;
	static CreateInstanceFn CreateInstance;
	static InvokeOnCreateFn InvokeOnCreate;
	static InvokeOnUpdateFn InvokeOnUpdate;
	static FreeInstanceFn FreeInstance;
};

} // namespace re::scripting