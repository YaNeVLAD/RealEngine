#include <Scripting/CSharp/DotNetInterop.hpp>

#define RE_SCRIPTING_API __declspec(dllexport)

re::scripting::DotNetInterop::LoadUserAssemblyFn re::scripting::DotNetInterop::LoadUserAssembly = nullptr;
re::scripting::DotNetInterop::CreateInstanceFn re::scripting::DotNetInterop::CreateInstance = nullptr;
re::scripting::DotNetInterop::InvokeOnCreateFn re::scripting::DotNetInterop::InvokeOnCreate = nullptr;
re::scripting::DotNetInterop::InvokeOnUpdateFn re::scripting::DotNetInterop::InvokeOnUpdate = nullptr;
re::scripting::DotNetInterop::FreeInstanceFn re::scripting::DotNetInterop::FreeInstance = nullptr;