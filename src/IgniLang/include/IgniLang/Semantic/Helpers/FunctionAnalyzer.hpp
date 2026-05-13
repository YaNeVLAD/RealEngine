#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
#include <memory>
#include <vector>

namespace igni::sem::FunctionAnalyzer
{

inline void AnalyzeBody(
	ast::IAstVisitor& visitor,
	SemanticContext& ctx,
	const std::shared_ptr<FunctionType>& funType,
	const std::vector<ast::Parameter>& astParams,
	const ast::Block* body,
	const bool isMethod,
	const std::shared_ptr<ClassType>& parentClass = nullptr,
	const std::function<void()>& injectExtras = nullptr)
{
	ctx.env.PushScope();

	if (injectExtras)
	{
		injectExtras();
	}

	if (isMethod && parentClass)
	{
		ctx.env.Define("this", parentClass, false);
	}

	for (std::size_t i = 0; i < astParams.size(); ++i)
	{
		const std::size_t typeIdx = isMethod ? i + 1 : i;
		std::shared_ptr<SemanticType> paramType = funType->paramTypes[typeIdx];

		// ВАЖНО: Массивы vararg уже сформированы на этапе объявления
		ctx.env.Define(astParams[i].name, paramType, false);
	}

	const auto previousReturnType = ctx.location.currentReturnType;
	const auto previousFunctionType = ctx.location.currentFunction;

	ctx.location.currentReturnType = funType->returnType;
	ctx.location.currentFunction = funType;

	if (body)
	{
		body->Accept(visitor);
	}

	if (!ctx.location.currentFunction->returnType)
	{
		ctx.location.currentFunction->returnType = ctx.tUnit;
	}
	funType->returnType = ctx.location.currentFunction->returnType;

	ctx.location.currentReturnType = previousReturnType;
	ctx.location.currentFunction = previousFunctionType;
	ctx.env.PopScope();
}

} // namespace igni::sem::FunctionAnalyzer