#pragma once

#include <Core/String.hpp>
#include <RVM/Types.hpp>

namespace re
{

struct ScriptComponent
{
	String className;
	rvm::Value instanceHandle = rvm::Null;
	bool isAwakeCalled = false;
};

} // namespace re