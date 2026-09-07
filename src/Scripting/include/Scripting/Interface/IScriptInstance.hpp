#pragma once

#include <Core/String.hpp>

namespace re::scripting
{

class IScriptInstance
{
public:
	virtual ~IScriptInstance() = default;

	virtual void InvokeMethod(String const& Method, void** args = nullptr) = 0;

	virtual void GetFieldValue(String const& Field, void* outValue) = 0;
};

} // namespace re::scripting
