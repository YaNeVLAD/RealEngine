#pragma once

#include <Core/String.hpp>
#include <vector>

namespace igni::sem::NameMangler
{

inline re::String Mangle(const re::String& baseName, const std::vector<re::String>& typeNames, bool isExternal)
{
	if (isExternal)
	{
		return baseName;
	}

	re::String mangled = baseName;
	for (const auto& tName : typeNames)
	{
		mangled = mangled + "@" + tName;
	}

	return mangled;
}

inline re::String MangleMethod(const re::String& className, const re::String& methodName, const std::vector<re::String>& typeNames, const bool isExternal)
{
	const re::String baseName = className + "_" + methodName;

	return Mangle(baseName, typeNames, isExternal);
}

inline re::String MangleGeneric(const re::String& baseName, const std::vector<re::String>& typeArgs)
{
	re::String mangled = baseName;
	for (const auto& tArg : typeArgs)
	{
		mangled = mangled + "@" + tArg;
	}

	return mangled;
}

inline re::String MangleDestructor(const re::String& className)
{
	return className + "_destructor";
}

} // namespace igni::sem::NameMangler