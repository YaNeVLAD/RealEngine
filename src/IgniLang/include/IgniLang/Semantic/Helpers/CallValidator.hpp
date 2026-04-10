#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::CallValidator
{

inline void ValidateArguments(const ast::CallExpr* node,
	const FunctionType* funType,
	const std::vector<std::shared_ptr<SemanticType>>& argTypes,
	const bool isMethodCall)
{
	const std::size_t thisOffset = isMethodCall ? 1 : 0;

	if (funType->isVararg)
	{
		const std::size_t fixedParams = funType->paramTypes.size() - 1 - thisOffset;
		if (argTypes.size() < fixedParams)
		{
			throw std::runtime_error("Semantic Error: Not enough arguments for vararg function '" + funType->name + "'");
		}

		const auto mutableNode = const_cast<ast::CallExpr*>(node);
		mutableNode->isVarargCall = true;
		mutableNode->varargCount = argTypes.size() - fixedParams;
	}
	else if (argTypes.size() + thisOffset != funType->paramTypes.size())
	{
		throw std::runtime_error("Semantic Error: Argument count mismatch for '" + funType->name + "'");
	}

	for (std::size_t i = 0; i < argTypes.size(); ++i)
	{
		std::shared_ptr<SemanticType> expectedType = (funType->isVararg && i + thisOffset >= funType->paramTypes.size() - 1)
			? funType->paramTypes.back()
			: funType->paramTypes[i + thisOffset];

		TypeResolver::ExpectAssignable(argTypes[i].get(), expectedType.get(), "argument " + std::to_string(i + 1));
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

		// Reserve place for 'this' argument for member functions
		const std::size_t paramOffset = (!funTmpl->astNode->parameters.empty() && funTmpl->astNode->parameters[0].name == "this") ? 1 : 0;
		for (std::size_t i = 0; i < argTypes.size() && (i + paramOffset) < funTmpl->astNode->parameters.size(); ++i)
		{
			const auto paramTypeAst = funTmpl->astNode->parameters[i + paramOffset].type.get();
			TypeResolver::InferTypeArguments(argTypes[i], paramTypeAst, funTmpl->typeParams, concreteArgs);
		}

		for (const auto& concreteArg : concreteArgs)
		{
			if (!concreteArg)
			{
				throw std::runtime_error("Semantic Error: Could not infer type argument for method '" + funTmpl->name + "'");
			}
		}
	}

	return ctx.instantiateFunctionCallback(funTmpl, concreteArgs);
}
} // namespace igni::sem::CallValidator