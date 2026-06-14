#pragma once

#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::MemberResolver
{

inline std::shared_ptr<SemanticType> ResolveAccess(
	const std::shared_ptr<ClassType>& classType,
	const re::String& memberName,
	const SemanticContext& ctx)
{
	const auto* node = classType->classDecl;

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
		IGNI_SEM_ERR("Class '" + classType->name + "' has no field or method named '" + memberName + "'");
	}

	if (vis == ast::Visibility::Private)
	{
		auto getBaseName = [](const re::String& name) {
			const std::size_t pos = name.Find('@');
			return pos != re::String::NPos ? name.Substring(0, pos) : name;
		};

		bool isInsideClass = false;

		if (ctx.location.currentClass && getBaseName(ctx.location.currentClass->name) == getBaseName(classType->name))
		{
			isInsideClass = true;
		}

		if (!isInsideClass)
		{
			if (const Symbol* thisSym = ctx.env.Resolve("this"))
			{
				if (const auto thisType = std::dynamic_pointer_cast<ClassType>(thisSym->type))
				{
					if (getBaseName(thisType->name) == getBaseName(classType->name))
					{
						isInsideClass = true;
					}
				}
			}
		}

		if (!isInsideClass)
		{
			IGNI_SEM_ERR("Cannot access private member '" + memberName + "' of class '" + classType->name + "'");
		}
	}
	else if (vis == ast::Visibility::Internal)
	{
		const re::String currentPkg = ctx.location.currentPackage ? ctx.location.currentPackage->name : "global";
		const re::String targetPkg = classType->moduleName.Empty() ? "global" : classType->moduleName;
		if (currentPkg != targetPkg)
		{
			IGNI_SEM_ERR("Cannot access internal member '" + memberName + "' outside of its package '" + targetPkg + "'");
		}
	}

	return resolvedType;
}

} // namespace igni::sem::MemberResolver