#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::CallValidator
{

inline void ValidateArguments(
	const ast::CallExpr* node,
	const FunctionType* funType,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	const bool isMethodCall,
	CallInfo& callInfo)
{
	const std::size_t thisOffset = isMethodCall ? 1 : 0;

	if (funType->isVararg)
	{
		const std::size_t fixedParams = funType->paramTypes.size() - 1 - thisOffset;
		if (argTypes.size() < fixedParams)
		{
			IGNI_SEM_ERR("Not enough arguments for vararg function '" + funType->name + "'");
		}

		callInfo.isVarargCall = true;
		callInfo.varargCount = argTypes.size() - fixedParams;
	}
	else if (argTypes.size() + thisOffset != funType->paramTypes.size())
	{
		IGNI_SEM_ERR("Argument count mismatch for '" + funType->name + "'");
	}

	for (std::size_t i = 0; i < argTypes.size(); ++i)
	{
		std::shared_ptr<SemanticType> expectedType = (funType->isVararg && i + thisOffset >= funType->paramTypes.size() - 1)
			? funType->paramTypes.back()
			: funType->paramTypes[i + thisOffset];

		TypeResolver::ExpectAssignable(argTypes[i].get(), expectedType.get(), "argument " + std::to_string(i + 1), node);
	}
}

inline std::shared_ptr<FunctionType> ResolveAndInstantiateGeneric(
	const std::shared_ptr<GenericFunctionTemplate>& funTmpl,
	const std::vector<std::shared_ptr<SemanticType>>& explicitTypeArgs,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	const SemanticContext& ctx)
{
	std::vector<std::shared_ptr<SemanticType>> concreteArgs;

	if (!explicitTypeArgs.empty())
	{ // ident::<Type>()
		concreteArgs = explicitTypeArgs;
	}
	else
	{ // ident(T)
		concreteArgs.resize(funTmpl->typeParams.size(), nullptr);

		for (std::size_t i = 0; i < argTypes.size() && i < funTmpl->astNode->parameters.size(); ++i)
		{
			const auto paramTypeAst = funTmpl->astNode->parameters[i].type.get();
			TypeResolver::InferTypeArguments(argTypes[i], paramTypeAst, funTmpl->typeParams, concreteArgs);
		}

		for (const auto& concreteArg : concreteArgs)
		{
			if (!concreteArg)
			{
				IGNI_SEM_ERR(funTmpl->astNode, "Semantic Error: Could not infer type argument for method '" + funTmpl->name + "'");
			}
		}
	}

	return ctx.instantiateFunctionCallback(funTmpl, concreteArgs);
}

inline bool IsApplicable(
	const FunctionType* funType,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	const bool isMethodCall)
{
	const std::size_t thisOffset = isMethodCall ? 1 : 0;

	if (funType->isVararg)
	{ // Wrong vararg arguments count
		if (const std::size_t fixedParams = funType->paramTypes.size() - 1 - thisOffset;
			argTypes.size() < fixedParams)
		{
			return false;
		}
	}
	else if (argTypes.size() + thisOffset != funType->paramTypes.size())
	{ // Wrong arguments count
		return false;
	}

	for (std::size_t i = 0; i < argTypes.size(); ++i)
	{
		std::shared_ptr<SemanticType> expectedType = (funType->isVararg && i + thisOffset >= funType->paramTypes.size() - 1)
			? funType->paramTypes.back()
			: funType->paramTypes[i + thisOffset];

		if (!argTypes[i]->IsAssignableTo(expectedType.get()))
		{ // Argument type mismatch
			return false;
		}
	}

	return true;
}

} // namespace igni::sem::CallValidator