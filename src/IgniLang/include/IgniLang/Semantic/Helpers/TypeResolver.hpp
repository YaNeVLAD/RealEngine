#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::TypeResolver
{

inline std::shared_ptr<SemanticType> Resolve(const ast::TypeNode* node, const SemanticContext& ctx)
{
	if (!node)
	{
		return nullptr;
	}

	std::shared_ptr<SemanticType> resolvedBaseType = nullptr;

	if (const auto simpleType = dynamic_cast<const ast::SimpleTypeNode*>(node))
	{
		using namespace re::literals;

		// clang-format off
		switch (simpleType->name.Hashed())
		{
		case "Int"_hs:    return ctx.tInt;
		case "Double"_hs: return ctx.tDouble;
		case "Bool"_hs:   return ctx.tBool;
		case "String"_hs: return ctx.tString;
		case "Unit"_hs:   return ctx.tUnit;
		case "Null"_hs:   return ctx.tNull;
		case "Any"_hs:    return ctx.tAny;
		default:          break;
		}
		// clang-format on

		if (!resolvedBaseType)
		{
			if (const Symbol* sym = ctx.env.Resolve(simpleType->name))
			{
				if (const auto classTmpl = std::dynamic_pointer_cast<GenericClassTemplate>(sym->type))
				{
					if (simpleType->typeArgs.empty())
					{
						throw std::runtime_error("Semantic Error: Generic class '" + simpleType->name + "' requires type arguments");
					}

					std::vector<std::shared_ptr<SemanticType>> concreteArgs;
					for (const auto& arg : simpleType->typeArgs)
					{
						concreteArgs.push_back(Resolve(arg.get(), ctx));
					}

					resolvedBaseType = ctx.instantiateClassCallback(classTmpl, concreteArgs);
				}
				else
				{
					resolvedBaseType = std::dynamic_pointer_cast<SemanticType>(sym->type);
				}
			}
			else if (ctx.location.currentPackage && ctx.location.currentPackage->exports.contains(simpleType->name))
			{
				resolvedBaseType = ctx.location.currentPackage->exports.at(simpleType->name);
			}
			else if (ctx.instantiatedClasses.contains(simpleType->name))
			{
				resolvedBaseType = ctx.instantiatedClasses.at(simpleType->name);
			}
			else
			{
				throw std::runtime_error("Semantic Error: Unknown type '" + simpleType->name + "'");
			}
		}
	}
	else if (const auto funTypeNode = dynamic_cast<const ast::FunctionTypeNode*>(node))
	{
		const auto semFunType = std::make_shared<FunctionType>("<anonymous_lambda>");
		for (const auto& pType : funTypeNode->paramTypes)
		{
			semFunType->paramTypes.push_back(Resolve(pType.get(), ctx));
		}
		semFunType->returnType = Resolve(funTypeNode->returnType.get(), ctx);

		resolvedBaseType = semFunType;
	}
	else
	{
		resolvedBaseType = ctx.tUnit;
	}

	if (node->isNullable)
	{
		if (resolvedBaseType->isNullable || resolvedBaseType == ctx.tNull)
		{
			return resolvedBaseType;
		}

		auto nullableType = resolvedBaseType->Clone();
		nullableType->isNullable = true;

		return nullableType;
	}

	return resolvedBaseType;
}

inline void ExpectAssignable(const SemanticType* actual, const SemanticType* expected, const std::string& contextMsg)
{
	if (!actual->IsAssignableTo(expected))
	{
		throw std::runtime_error("Semantic Error: Type mismatch in " + contextMsg + ". Expected " + expected->name + ", got " + actual->name);
	}
}

inline std::shared_ptr<SemanticType> PromoteMathTypes(const SemanticType* lhs, const SemanticType* rhs, SemanticContext& ctx)
{
	const bool isNumeric = (lhs == ctx.tInt.get() || lhs == ctx.tDouble.get()) && (rhs == ctx.tInt.get() || rhs == ctx.tDouble.get());

	if (isNumeric)
	{
		return (lhs == ctx.tDouble.get() || rhs == ctx.tDouble.get()) ? ctx.tDouble : ctx.tInt;
	}

	ExpectAssignable(rhs, lhs, "binary expression");

	return nullptr;
}

inline void InferTypeArguments(const std::shared_ptr<SemanticType>& argType,
	const ast::TypeNode* paramTypeAst,
	const std::vector<ast::GenericTypeParam>& typeParams,
	std::vector<std::shared_ptr<SemanticType>>& outConcreteArgs)
{
	if (!argType || !paramTypeAst)
	{
		return;
	}

	if (const auto simpleAst = dynamic_cast<const ast::SimpleTypeNode*>(paramTypeAst))
	{
		for (std::size_t k = 0; k < typeParams.size(); ++k)
		{ // Basic case: T is T
			if (typeParams[k].name == simpleAst->name)
			{
				if (!outConcreteArgs[k])
				{
					outConcreteArgs[k] = argType;
				}
				return;
			}
		}

		if (!simpleAst->typeArgs.empty())
		{ // Recursive case 1: T is generic argument of Class<T>
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(argType))
			{
				for (std::size_t i = 0; i < simpleAst->typeArgs.size() && i < classType->typeArguments.size(); ++i)
				{
					InferTypeArguments(classType->typeArguments[i], simpleAst->typeArgs[i].get(), typeParams, outConcreteArgs);
				}
			}
		}
	}
	else if (const auto funAst = dynamic_cast<const ast::FunctionTypeNode*>(paramTypeAst))
	{ // Recursive case 2: T is generic function argument like callback: (T) -> Unit
		if (const auto funType = std::dynamic_pointer_cast<FunctionType>(argType))
		{
			for (std::size_t i = 0; i < funAst->paramTypes.size() && i < funType->paramTypes.size(); ++i)
			{
				InferTypeArguments(funType->paramTypes[i], funAst->paramTypes[i].get(), typeParams, outConcreteArgs);
			}
			InferTypeArguments(funType->returnType, funAst->returnType.get(), typeParams, outConcreteArgs);
		}
	}
}

} // namespace igni::sem::TypeResolver