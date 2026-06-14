#pragma once

#include <Core/String.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Declaration::Overload
{

inline void Global(const re::String& originalName, const std::shared_ptr<FunctionType>& funType, SemanticContext& ctx, const std::shared_ptr<ModuleType>& module)
{
	std::shared_ptr<FunctionGroup> group;
	if (const Symbol* sym = ctx.env.Resolve(originalName))
	{
		group = std::dynamic_pointer_cast<FunctionGroup>(sym->type);
	}

	if (!group)
	{
		group = std::make_shared<FunctionGroup>(originalName);
		ctx.env.Define(originalName, group, true);
		if (module)
		{
			module->exports[originalName] = group;
		}
	}
	group->overloads.push_back(funType);
}

inline void GenericGlobal(const re::String& originalName, const std::shared_ptr<GenericFunctionTemplate>& tmpl, SemanticContext& ctx, const std::shared_ptr<ModuleType>& module)
{
	std::shared_ptr<FunctionGroup> group;
	if (const Symbol* sym = ctx.env.Resolve(originalName))
	{
		group = std::dynamic_pointer_cast<FunctionGroup>(sym->type);
	}

	if (!group)
	{
		group = std::make_shared<FunctionGroup>(originalName);
		ctx.env.Define(originalName, group, true);
		if (module)
		{
			module->exports[originalName] = group;
		}
	}
	group->templates.push_back(tmpl);
}

inline void Method(const std::shared_ptr<ClassType>& classType, const re::String& originalName, const std::shared_ptr<FunctionType>& funType)
{
	std::shared_ptr<FunctionGroup> group;
	if (classType->methods.contains(originalName))
	{
		group = std::dynamic_pointer_cast<FunctionGroup>(classType->methods[originalName]);
	}

	if (!group)
	{
		group = std::make_shared<FunctionGroup>(originalName);
		classType->methods[originalName] = group;
	}

	group->overloads.push_back(funType);
}

inline void GenericMethod(const std::shared_ptr<ClassType>& classType, const re::String& originalName, const std::shared_ptr<GenericFunctionTemplate>& tmpl)
{
	std::shared_ptr<FunctionGroup> group;
	if (classType->methods.contains(originalName))
	{
		group = std::dynamic_pointer_cast<FunctionGroup>(classType->methods[originalName]);
	}

	if (!group)
	{
		group = std::make_shared<FunctionGroup>(originalName);
		classType->methods[originalName] = group;
	}
	group->templates.push_back(tmpl);
}

} // namespace igni::sem::Declaration::Overload