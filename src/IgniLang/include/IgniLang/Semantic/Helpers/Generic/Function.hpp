#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
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
		IGNI_SEM_ERR(tmpl->astNode, "Generic function '" + tmpl->name + "' expected " + std::to_string(tmpl->typeParams.size()) + " type arguments, but got " + std::to_string(typeArgs.size()));
	}

	re::String uniqueName = tmpl->name;
	for (const auto& arg : typeArgs)
	{
		if (!arg)
		{
			IGNI_SEM_ERR(tmpl->astNode, "Invalid or unknown type argument for '" + tmpl->name + "'");
		}

		uniqueName = uniqueName + "@" + arg->name;
	}

	if (m_context.instantiatedFunctions.contains(uniqueName))
	{
		return m_context.instantiatedFunctions[uniqueName];
	}

	ast::TypeEnv typeEnv;
	std::vector<std::unique_ptr<ast::SimpleTypeNode>> tempNodes;

	for (std::size_t i = 0; i < typeArgs.size(); ++i)
	{
		if (tmpl->typeParams[i].boundType)
		{
			auto boundSemType = TypeResolver::Resolve(tmpl->typeParams[i].boundType.get(), m_context);
			TypeResolver::ExpectAssignable(typeArgs[i].get(), boundSemType.get(), "type parameter bound", tmpl->astNode);
		}
		auto tempNode = std::make_unique<ast::SimpleTypeNode>();
		tempNode->name = typeArgs[i]->name;
		typeEnv[tmpl->typeParams[i].name] = tempNode.get();
		tempNodes.emplace_back(std::move(tempNode));
	}

	auto clonedDecl = tmpl->astNode->CloneDecl(&typeEnv);
	const auto realFunDecl = dynamic_cast<ast::FunDecl*>(clonedDecl.get());
	realFunDecl->name = uniqueName;
	realFunDecl->typeParams.clear();

	auto funType = Declaration::Function(realFunDecl, m_context, tmpl->moduleName);

	m_context.instantiatedFunctions[uniqueName] = funType;
	m_context.env.Define(uniqueName, funType, true);

	m_context.m_pendingFunInstantiations.emplace_back(realFunDecl);
	m_context.m_instantiatedNodes.insert(realFunDecl);
	m_context.pendingStatements.emplace_back(std::move(clonedDecl));

	return funType;
}

} // namespace igni::sem::Generic::Function