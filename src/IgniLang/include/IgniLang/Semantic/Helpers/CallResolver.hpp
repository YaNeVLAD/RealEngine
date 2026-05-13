#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/CallValidator.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>

namespace igni::sem::CallResolver
{

namespace detail
{
// --- 1. Обработка встроенных интринсиков компилятора ---
inline std::shared_ptr<SemanticType> TryProcessIntrinsic(
	const ast::CallExpr* node, SemanticContext& ctx, CallInfo& callInfo)
{
	const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get());
	if (!id)
		return nullptr;

	if (id->name == "typeof" && node->arguments.size() == 1)
	{
		callInfo.dispatchMode = CallDispatchType::TypeOf;
		ctx.bindings.callInfo[node] = callInfo;
		return ctx.tType;
	}
	if (id->name == "Type" && !id->typeArgs.empty() && node->arguments.empty())
	{
		auto targetType = TypeResolver::Resolve(id->typeArgs[0].get(), ctx);
		if (!targetType)
			IGNI_SEM_ERR(node, "Unknown type in Type::<T>()");

		callInfo.dispatchMode = CallDispatchType::TypeLiteral;
		callInfo.asmLabel = targetType->name;
		ctx.bindings.callInfo[node] = callInfo;
		return ctx.tType;
	}

	return nullptr;
}

// --- 2. Поиск лучшей перегрузки ---
inline std::shared_ptr<FunctionType> FindBestOverload(
	const ast::CallExpr* node,
	const std::shared_ptr<FunctionGroup>& group,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	bool isMethod)
{
	std::shared_ptr<FunctionType> bestMatch = nullptr;
	int matchCount = 0;
	for (const auto& overload : group->overloads)
	{
		if (CallValidator::IsApplicable(overload.get(), argTypes, isMethod))
		{
			bestMatch = overload;
			matchCount++;
		}
	}
	if (matchCount > 1)
		IGNI_SEM_ERR(node, "Ambiguous call: multiple matching overloads found");
	return bestMatch;
}

struct TargetResolution
{
	std::shared_ptr<FunctionType> target = nullptr;
	bool isMethodCall = false;
	bool isIndirectCall = false;
	bool isGenericInstantiation = false;
};

// --- 3. Разрешение целевой функции/метода ---
inline TargetResolution ResolveTarget(
	const ast::CallExpr* node,
	SemanticContext& ctx,
	const std::shared_ptr<SemanticType>& calleeType,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	std::vector<std::shared_ptr<SemanticType>>& explicitTypeArgs,
	CallInfo& callInfo)
{
	TargetResolution res;

	if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(calleeType))
	{ // Конструктор дженерик-класса
		if (explicitTypeArgs.empty())
		{
			const ast::ConstructorDecl* ctorAst = nullptr;
			for (const auto& member : classTmpl->astNode->members)
			{
				if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
				{
					ctorAst = ctor;
					break;
				}
			}

			if (ctorAst)
			{
				explicitTypeArgs.resize(classTmpl->typeParams.size(), nullptr);
				for (std::size_t i = 0; i < argTypes.size() && i < ctorAst->parameters.size(); ++i)
				{
					TypeResolver::InferTypeArguments(argTypes[i], ctorAst->parameters[i].type.get(), classTmpl->typeParams, explicitTypeArgs);
				}
				for (const auto& arg : explicitTypeArgs)
				{
					if (!arg)
						IGNI_SEM_ERR(node, "Could not infer type arguments for generic class '" + classTmpl->name + "'. Use explicit ::<T> syntax.");
				}
			}
			else
			{
				IGNI_SEM_ERR(node, "Generic class '" + classTmpl->name + "' has no parameters to infer types from.");
			}
		}

		const auto concreteClass = ctx.instantiateClassCallback(classTmpl, explicitTypeArgs);
		const auto it = concreteClass->methods.find(concreteClass->name);
		if (it == concreteClass->methods.end())
			IGNI_SEM_ERR(node, "Constructor not found for generic class '" + concreteClass->name + "'");

		if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(it->second))
			res.target = FindBestOverload(node, group, argTypes, true);

		callInfo.isConstructorCall = true;
		callInfo.mangledClassName = classTmpl->astNode->isExternal ? classTmpl->name : concreteClass->name;
		res.isMethodCall = true;

		if (const auto id = dynamic_cast<ast::IdentifierExpr*>(node->callee.get()))
			id->name = classTmpl->astNode->isExternal ? classTmpl->name : concreteClass->name;
	}
	else if (const auto classType = std::dynamic_pointer_cast<ClassType>(calleeType))
	{ // Обычный конструктор
		const auto it = classType->methods.find(classType->name);
		if (it == classType->methods.end())
			IGNI_SEM_ERR(node, "Constructor not found for class '" + classType->name + "'");

		if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(it->second))
			res.target = FindBestOverload(node, group, argTypes, true);

		callInfo.isConstructorCall = true;
		callInfo.mangledClassName = classType->name;
		res.isMethodCall = true;
	}
	else if (const auto group = std::dynamic_pointer_cast<FunctionGroup>(calleeType))
	{ // Обычный метод или функция (перегрузки)
		res.isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr || callInfo.isImplicitThisCall;
		res.target = FindBestOverload(node, group, argTypes, res.isMethodCall);

		if (!res.target && !group->templates.empty())
		{
			res.target = CallValidator::ResolveAndInstantiateGeneric(group->templates[0], explicitTypeArgs, argTypes, ctx);
			res.isGenericInstantiation = true;
		}
	}
	else if (const auto funType = std::dynamic_pointer_cast<FunctionType>(calleeType))
	{ // Переменная-лямбда
		res.isMethodCall = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()) != nullptr;
		res.target = funType;
		res.isIndirectCall = true;

		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{
			if (ctx.bindings.resolvedNames.contains(id))
				res.isIndirectCall = false;
		}
		else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			if (ctx.bindings.resolvedMembers.contains(memAccess))
				res.isIndirectCall = false;
		}
	}
	else
	{
		IGNI_SEM_ERR(node, "Attempt to call a non-callable type");
	}

	if (!res.target)
		IGNI_SEM_ERR(node, "No matching overload found");
	return res;
}

// --- 4. Определение типа вызова ВМ ---
inline void DetermineDispatchMode(
	const ast::CallExpr* node,
	const TargetResolution& res,
	CallInfo& callInfo)
{
	re::String rawTargetName = "";
	if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		rawTargetName = id->name;
	else if (const auto mem = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		rawTargetName = mem->member;

	if (res.isIndirectCall)
	{
		callInfo.dispatchMode = CallDispatchType::Indirect;
	}
	else if (callInfo.isSuperCall)
	{
		callInfo.dispatchMode = CallDispatchType::Static;
		callInfo.asmLabel = res.target->name;
	}
	else if (callInfo.isConstructorCall)
	{
		callInfo.dispatchMode = res.target->isExternal ? CallDispatchType::Virtual : CallDispatchType::Static;
		callInfo.asmLabel = res.target->isExternal ? "init" : res.target->name;
	}
	else
	{
		bool isExtClassMethod = false;
		if (res.isMethodCall && !res.target->paramTypes.empty())
		{
			if (auto ct = std::dynamic_pointer_cast<ClassType>(res.target->paramTypes[0]))
				isExtClassMethod = ct->classDecl ? ct->classDecl->isExternal : true;
			else if (auto gt = std::dynamic_pointer_cast<GenericClassTemplate>(res.target->paramTypes[0]))
				isExtClassMethod = gt->astNode ? gt->astNode->isExternal : true;
		}

		if (res.target->isExternal)
		{
			callInfo.dispatchMode = res.isMethodCall ? CallDispatchType::Virtual : CallDispatchType::Native;
			callInfo.asmLabel = rawTargetName;
		}
		else if (res.isMethodCall && !res.isGenericInstantiation && !isExtClassMethod)
		{
			callInfo.dispatchMode = CallDispatchType::Virtual;
			callInfo.asmLabel = rawTargetName;
		}
		else
		{
			callInfo.dispatchMode = CallDispatchType::Static;
			callInfo.asmLabel = res.target->name;
		}
	}
}
} // namespace detail

// ===============================================
// ФАСАД (Точка входа)
// ===============================================
inline std::shared_ptr<SemanticType> Process(const ast::CallExpr* node, SemanticContext& ctx)
{
	CallInfo callInfo;
	callInfo.isSuperCall = false;

	const auto calleeType = ctx.evaluateFunctionCallback(node->callee.get());
	std::vector<std::shared_ptr<SemanticType>> argTypes;
	for (const auto& arg : node->arguments)
	{
		argTypes.emplace_back(ctx.evaluateFunctionCallback(arg.get()));
	}

	std::vector<std::shared_ptr<SemanticType>> explicitTypeArgs;
	if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
	{ // Explicit generic arguments T::<U>
		using namespace re::literals;
		callInfo.isSuperCall = id->name.Hashed() == "super"_hs;
		if (ctx.bindings.implicitThisNames.contains(id))
		{
			callInfo.isImplicitThisCall = true;
		}
		for (const auto& t : id->typeArgs)
		{
			explicitTypeArgs.push_back(TypeResolver::Resolve(t.get(), ctx));
		}
	}
	else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
	{
		for (const auto& t : memAccess->typeArgs)
		{
			explicitTypeArgs.push_back(TypeResolver::Resolve(t.get(), ctx));
		}
	}

	if (auto intrinsicType = detail::TryProcessIntrinsic(node, ctx, callInfo))
	{
		return intrinsicType;
	}

	auto resolution = detail::ResolveTarget(node, ctx, calleeType, argTypes, explicitTypeArgs, callInfo);
	callInfo.target = resolution.target;

	if (resolution.target->isSuspend && !ctx.location.isInsideLaunch)
	{
		if (!ctx.location.currentFunction || !ctx.location.currentFunction->isSuspend)
		{
			IGNI_SEM_ERR(node, "Suspend function '" + resolution.target->name + "' can only be called from a coroutine.");
		}
	}

	if (callInfo.isSuperCall)
	{
		const Symbol* sym = ctx.env.Resolve("this");
		if (!sym)
		{
			IGNI_SEM_ERR(node, "Undefined variable 'this'");
		}

		argTypes.insert(argTypes.begin(), sym->type);
		callInfo.isConstructorCall = false;
		resolution.isMethodCall = false;
	}

	detail::DetermineDispatchMode(node, resolution, callInfo);

	CallValidator::ValidateArguments(node, resolution.target.get(), argTypes, resolution.isMethodCall, callInfo);
	ctx.bindings.callInfo[node] = callInfo;

	if (callInfo.isConstructorCall)
	{
		return std::dynamic_pointer_cast<ClassType>(calleeType) ? calleeType : resolution.target->paramTypes[0];
	}

	if (!resolution.target->returnType)
	{
		IGNI_SEM_ERR(node, "Cannot infer return type for forward call");
	}

	return resolution.target->returnType;
}

} // namespace igni::sem::CallResolver