#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Generic::Function
{

inline std::shared_ptr<FunctionType> Instantiate(
	const std::shared_ptr<GenericFunctionTemplate>& tmpl,
	const std::vector<std::shared_ptr<SemanticType>>& typeArgs,
	SemanticContext& m_context)
{
	if (typeArgs.size() != tmpl->typeParams.size())
	{
		throw std::runtime_error("Semantic Error: Generic function '" + tmpl->name + "' expected " + std::to_string(tmpl->typeParams.size()) + " type arguments, but got " + std::to_string(typeArgs.size()));
	}

	re::String uniqueName = tmpl->name;
	if (!tmpl->isExternal)
	{
		for (const auto& arg : typeArgs)
		{
			if (!arg)
			{
				throw std::runtime_error("Semantic Error: Invalid or unknown type argument for '" + tmpl->name + "'");
			}

			uniqueName = uniqueName + "__" + arg->name;
		}

		if (m_context.instantiatedFunctions.contains(uniqueName))
		{
			return m_context.instantiatedFunctions[uniqueName];
		}
	}

	ast::TypeEnv typeEnv;
	std::vector<std::unique_ptr<ast::SimpleTypeNode>> tempNodes;

	for (std::size_t i = 0; i < typeArgs.size(); ++i)
	{
		if (tmpl->typeParams[i].boundType)
		{
			auto boundSemType = TypeResolver::Resolve(tmpl->typeParams[i].boundType.get(), m_context);
			TypeResolver::ExpectAssignable(typeArgs[i].get(), boundSemType.get(), "type parameter bound");
		}
		auto tempNode = std::make_unique<ast::SimpleTypeNode>();
		tempNode->name = typeArgs[i]->name;
		typeEnv[tmpl->typeParams[i].name] = tempNode.get();
		tempNodes.push_back(std::move(tempNode));
	}

	auto clonedDecl = tmpl->astNode->CloneDecl(&typeEnv);
	const auto realFunDecl = dynamic_cast<ast::FunDecl*>(clonedDecl.get());
	realFunDecl->name = uniqueName;
	realFunDecl->typeParams.clear();

	auto funType = std::make_shared<FunctionType>(uniqueName);
	funType->moduleName = tmpl->moduleName;
	funType->isVararg = realFunDecl->isVararg;
	funType->visibility = realFunDecl->visibility;
	funType->isExternal = tmpl->isExternal;

	for (const auto& p : realFunDecl->parameters)
	{
		funType->paramTypes.push_back(TypeResolver::Resolve(p.type.get(), m_context));
	}
	funType->returnType = TypeResolver::Resolve(realFunDecl->returnType.get(), m_context);

	if (funType->returnType == nullptr && realFunDecl->isExprBody)
	{ // On-demand type evaluation
		m_context.env.PushScope();
		for (std::size_t j = 0; j < realFunDecl->parameters.size(); ++j)
		{
			m_context.env.Define(realFunDecl->parameters[j].name, funType->paramTypes[j], false);
		}

		const auto prevFun = m_context.location.currentFunction;
		m_context.location.currentFunction = funType;

		if (realFunDecl->body && !realFunDecl->body->statements.empty())
		{
			if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(realFunDecl->body->statements[0].get()))
				funType->returnType = m_context.evaluateFunctionCallback(retStmt->expr.get());
		}
		m_context.location.currentFunction = prevFun;
		m_context.env.PopScope();
	}
	else if (funType->returnType == nullptr && !realFunDecl->isExprBody)
	{
		funType->returnType = m_context.tUnit;
	}

	m_context.instantiatedFunctions[uniqueName] = funType;
	m_context.env.Define(uniqueName, funType, true);
	m_context.allFunctionNames.insert(uniqueName);

	if (funType->isExternal)
	{
		m_context.externalFunctions.insert(uniqueName);
	}

	m_context.m_pendingFunInstantiations.push_back(realFunDecl);
	m_context.m_instantiatedNodes.insert(realFunDecl);

	m_context.pendingStatements.push_back(std::move(clonedDecl));

	return funType;
}

} // namespace igni::sem::Generic::Function