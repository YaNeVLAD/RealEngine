#pragma once

#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::MemberResolver
{

inline std::shared_ptr<SemanticType> ResolveAccess(const std::shared_ptr<ClassType>& classType,
	const re::String& memberName,
	const SemanticContext& ctx)
{
	auto vis = ast::Visibility::Public;
	std::shared_ptr<SemanticType> resolvedType = nullptr;

	if (const auto fieldIt = classType->fields.find(memberName); fieldIt != classType->fields.end())
	{
		resolvedType = fieldIt->second.type;
		vis = fieldIt->second.visibility;
	}
	else if (const auto methodIt = classType->methods.find(memberName); methodIt != classType->methods.end())
	{
		resolvedType = methodIt->second;
		if (const auto funType = std::dynamic_pointer_cast<FunctionType>(resolvedType))
		{
			vis = funType->visibility;
		}
		else if (const auto tmplType = std::dynamic_pointer_cast<GenericFunctionTemplate>(resolvedType))
		{
			vis = tmplType->visibility;
		}
	}
	else
	{
		throw std::runtime_error("Semantic Error: Class '" + classType->name + "' has no field or method named '" + memberName + "'");
	}

	if (vis == ast::Visibility::Private)
	{
		if (!ctx.location.currentClass || ctx.location.currentClass->name != classType->name)
		{
			throw std::runtime_error("Semantic Error: Cannot access private member '" + memberName + "' of class '" + classType->name + "'");
		}
	}
	else if (vis == ast::Visibility::Internal)
	{
		const re::String currentPkg = ctx.location.currentPackage ? ctx.location.currentPackage->name : "global";
		const re::String targetPkg = classType->moduleName.Empty() ? "global" : classType->moduleName;
		if (currentPkg != targetPkg)
		{
			throw std::runtime_error("Semantic Error: Cannot access internal member '" + memberName + "' outside of its package '" + targetPkg + "'");
		}
	}

	return resolvedType;
}

} // namespace igni::sem::MemberResolver