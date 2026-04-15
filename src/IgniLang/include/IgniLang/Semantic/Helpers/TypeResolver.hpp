#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
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
		case "Int"_hs:    resolvedBaseType = ctx.tInt; break;
		case "Double"_hs: resolvedBaseType = ctx.tDouble; break;
		case "Bool"_hs:   resolvedBaseType = ctx.tBool; break;
		case "String"_hs: resolvedBaseType = ctx.tString; break;
		case "Unit"_hs:   resolvedBaseType = ctx.tUnit; break;
		case "Null"_hs:   resolvedBaseType = ctx.tNull; break;
		case "Any"_hs:    resolvedBaseType = ctx.tAny; break;
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
						IGNI_SEM_ERR("Generic class '" + simpleType->name + "' requires type arguments");
					}

					std::vector<std::shared_ptr<SemanticType>> concreteArgs;
					for (const auto& arg : simpleType->typeArgs)
					{
						concreteArgs.emplace_back(Resolve(arg.get(), ctx));
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
				IGNI_SEM_ERR("Unknown type '" + simpleType->name + "'");
			}
		}
	}
	else if (const auto funTypeNode = dynamic_cast<const ast::FunctionTypeNode*>(node))
	{
		const auto semFunType = std::make_shared<FunctionType>("<anonymous_lambda>");
		for (const auto& pType : funTypeNode->paramTypes)
		{
			semFunType->paramTypes.emplace_back(Resolve(pType.get(), ctx));
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

inline void ExpectAssignable(const SemanticType* actual, const SemanticType* expected, const std::string& contextMsg, const ast::Node* node = nullptr)
{
	if (!actual->IsAssignableTo(expected))
	{
		IGNI_SEM_ERR("Type mismatch in " + contextMsg + ". Expected " + expected->name + (expected->isNullable ? "?" : "") + ", got " + actual->name + (actual->isNullable ? "?" : ""));
	}
}

inline std::shared_ptr<SemanticType> PromoteMathTypes(const SemanticType* lhs, const SemanticType* rhs, const ast::BinaryExpr* node, SemanticContext& ctx)
{
	const bool isNumeric = (lhs == ctx.tInt.get() || lhs == ctx.tDouble.get()) && (rhs == ctx.tInt.get() || rhs == ctx.tDouble.get());

	if (isNumeric)
	{
		return (lhs == ctx.tDouble.get() || rhs == ctx.tDouble.get()) ? ctx.tDouble : ctx.tInt;
	}

	ExpectAssignable(rhs, lhs, "binary expression", node);

	return nullptr;
}

inline void InferTypeArguments(const std::shared_ptr<SemanticType>& argType,
	const ast::TypeNode* paramTypeAst,
	const std::vector<ast::GenericTypeParam>& typeParams,
	std::vector<std::shared_ptr<SemanticType>>& outTypeArgs)
{
	if (!argType || !paramTypeAst)
	{
		return;
	}

	if (const auto simpleParam = dynamic_cast<const ast::SimpleTypeNode*>(paramTypeAst))
	{
		const auto it = std::ranges::find_if(typeParams,
			[&](const auto& p) { return p.name == simpleParam->name; });

		if (it != typeParams.end())
		{ // T = T
			const std::size_t index = std::distance(typeParams.begin(), it);

			if (!outTypeArgs[index])
			{
				outTypeArgs[index] = argType;
			}
		}
		else if (!simpleParam->typeArgs.empty())
		{ // T = class <T> or fun <T>
			if (const auto classType = std::dynamic_pointer_cast<ClassType>(argType))
			{
				if (classType->typeArguments.size() == simpleParam->typeArgs.size())
				{
					for (std::size_t i = 0; i < simpleParam->typeArgs.size(); ++i)
					{
						InferTypeArguments(classType->typeArguments[i], simpleParam->typeArgs[i].get(), typeParams, outTypeArgs);
					}
				}
			}
		}
	}
	else if (const auto funParam = dynamic_cast<const ast::FunctionTypeNode*>(paramTypeAst))
	{ // T = (T, S, ...) -> R
		if (const auto funType = std::dynamic_pointer_cast<FunctionType>(argType))
		{
			for (std::size_t i = 0; i < funParam->paramTypes.size() && i < funType->paramTypes.size(); ++i)
			{
				InferTypeArguments(funType->paramTypes[i], funParam->paramTypes[i].get(), typeParams, outTypeArgs);
			}
			InferTypeArguments(funType->returnType, funParam->returnType.get(), typeParams, outTypeArgs);
		}
	}
}

} // namespace igni::sem::TypeResolver