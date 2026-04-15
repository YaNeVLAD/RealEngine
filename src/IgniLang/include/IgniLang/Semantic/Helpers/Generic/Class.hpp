#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>
#include <IgniLang/Semantic/SemanticError.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

namespace igni::sem::Generic::Class
{

inline std::shared_ptr<ClassType> Instantiate(
	const std::shared_ptr<GenericClassTemplate>& tmpl,
	const std::vector<std::shared_ptr<SemanticType>>& typeArgs,
	SemanticContext& m_context)
{
	if (typeArgs.size() != tmpl->typeParams.size())
	{
		IGNI_SEM_ERR(tmpl->astNode, "Generic class '" + tmpl->name + "' expected " + std::to_string(tmpl->typeParams.size()) + " type arguments, but got " + std::to_string(typeArgs.size()));
	}

	re::String uniqueName = tmpl->name;
	for (const auto& arg : typeArgs)
	{
		if (!arg)
		{
			IGNI_SEM_ERR(tmpl->astNode, "Invalid or unknown type argument for '" + tmpl->name + "'");
		}

		uniqueName = uniqueName + "__" + arg->name;
	}

	if (m_context.instantiatedClasses.contains(uniqueName))
	{
		return m_context.instantiatedClasses[uniqueName];
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
	auto realClassDecl = dynamic_cast<ast::ClassDecl*>(clonedDecl.get());
	realClassDecl->name = uniqueName;
	realClassDecl->typeParams.clear();

	bool hasConstructor = false;
	for (const auto& member : realClassDecl->members)
	{
		if (dynamic_cast<const ast::ConstructorDecl*>(member.get()))
		{
			hasConstructor = true;
			break;
		}
	}
	if (!hasConstructor)
	{
		auto defaultCtor = std::make_unique<ast::ConstructorDecl>();
		defaultCtor->name = uniqueName;
		defaultCtor->body = std::make_unique<ast::Block>();
		realClassDecl->members.push_back(std::move(defaultCtor));
	}

	auto classType = std::make_shared<ClassType>(uniqueName, tmpl->astNode);
	classType->classDecl = realClassDecl;
	classType->moduleName = tmpl->moduleName;
	classType->typeArguments = typeArgs;
	m_context.instantiatedClasses[uniqueName] = classType;

	m_context.env.Define(uniqueName, classType, true);
	for (const auto& member : realClassDecl->members)
	{
		if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
		{
			auto fieldType = TypeResolver::Resolve(varDecl->type.get(), m_context);
			if (varDecl->initializer)
			{
				const auto initType = m_context.evaluateFunctionCallback(varDecl->initializer.get());
				if (!fieldType || fieldType == m_context.tUnit)
				{
					fieldType = initType;
				}
			}
			else if (!varDecl->isExternal && !realClassDecl->isExternal && !fieldType)
			{
				IGNI_SEM_ERR(varDecl, "Property '" + varDecl->name + "' must have an initializer or explicit type");
			}

			classType->fields[varDecl->name] = { fieldType, false, varDecl->visibility };
		}
		else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
		{
			auto fieldType = TypeResolver::Resolve(valDecl->type.get(), m_context);
			if (valDecl->initializer)
			{
				const auto initType = m_context.evaluateFunctionCallback(valDecl->initializer.get());
				if (!fieldType || fieldType == m_context.tUnit)
				{
					fieldType = initType;
				}
			}
			else if (!valDecl->isExternal && !realClassDecl->isExternal && !fieldType)
			{
				IGNI_SEM_ERR(valDecl, "Property '" + valDecl->name + "' must have an initializer or explicit type");
			}

			classType->fields[valDecl->name] = { fieldType, false, valDecl->visibility };
		}
		if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
		{
			const auto mutableFun = const_cast<ast::FunDecl*>(fun);
			const re::String originalName = Declaration::InjectThisKeyword(mutableFun, uniqueName);

			if (!mutableFun->typeParams.empty())
			{
				if (mutableFun->isOverride)
				{
					IGNI_SEM_ERR(mutableFun, "Generic methods cannot be marked 'override'");
				}

				const auto fnTmpl = std::make_shared<GenericFunctionTemplate>(mutableFun->name);
				fnTmpl->astNode = mutableFun;
				fnTmpl->typeParams = mutableFun->typeParams;
				fnTmpl->moduleName = tmpl->moduleName;
				fnTmpl->visibility = mutableFun->visibility;
				fnTmpl->isExternal = fun->isExternal;

				classType->methods[originalName] = fnTmpl;
				continue;
			}

			Declaration::Method(mutableFun, originalName, classType, m_context);
		}
		else if (const auto ctor = dynamic_cast<ast::ConstructorDecl*>(member.get()))
		{
			Declaration::Constructor(ctor, classType, realClassDecl->isExternal, m_context);
		}
	}

	m_context.m_pendingClassInstantiations.push_back(realClassDecl);
	m_context.m_instantiatedNodes.insert(realClassDecl);
	for (const auto& member : realClassDecl->members)
	{
		m_context.m_instantiatedNodes.insert(member.get());
	}

	m_context.pendingStatements.push_back(std::move(clonedDecl));

	return classType;
}

} // namespace igni::sem::Generic::Class