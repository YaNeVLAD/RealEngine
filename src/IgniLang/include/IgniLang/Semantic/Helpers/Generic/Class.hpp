#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>
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
		throw std::runtime_error("Semantic Error: Generic class '" + tmpl->name + "' expected " + std::to_string(tmpl->typeParams.size()) + " type arguments, but got " + std::to_string(typeArgs.size()));
	}

	re::String uniqueName = tmpl->name;
	for (const auto& arg : typeArgs)
	{
		if (!arg)
		{
			throw std::runtime_error("Semantic Error: Invalid or unknown type argument for '" + tmpl->name + "'");
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
				throw std::runtime_error("Semantic Error: Property '" + varDecl->name + "' must have an initializer or explicit type");
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
				throw std::runtime_error("Semantic Error: Property '" + valDecl->name + "' must have an initializer or explicit type");
			}

			classType->fields[valDecl->name] = { fieldType, false, valDecl->visibility };
		}
		if (const auto fun = dynamic_cast<const ast::FunDecl*>(member.get()))
		{
			if (realClassDecl->isExternal && (!fun->isExternal || fun->body || fun->isExprBody))
			{
				throw std::runtime_error("Semantic Error: Method '" + fun->name + "' in external class '" + realClassDecl->name + "' must be marked 'external' and cannot have a body.");
			}

			const auto mutableFun = const_cast<ast::FunDecl*>(fun);
			const re::String originalName = mutableFun->name;

			if (mutableFun->parameters.empty() || mutableFun->parameters[0].name != "this")
			{
				ast::FunDecl::Parameter thisParam;
				thisParam.name = "this";
				auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
				thisTypeNode->name = uniqueName;
				thisParam.type = std::move(thisTypeNode);
				mutableFun->parameters.insert(mutableFun->parameters.begin(), std::move(thisParam));
			}

			mutableFun->name = uniqueName + "_" + originalName;
			m_context.allFunctionNames.insert(mutableFun->name);

			if (!mutableFun->typeParams.empty())
			{
				if (mutableFun->isOverride)
				{
					throw std::runtime_error("Semantic Error: Generic methods cannot be marked 'override' (restricted by monomorphization). Method: '" + originalName + "'");
				}

				auto fnTmpl = std::make_shared<GenericFunctionTemplate>(mutableFun->name);
				fnTmpl->astNode = mutableFun;
				fnTmpl->typeParams = mutableFun->typeParams;
				fnTmpl->moduleName = tmpl->moduleName;

				fnTmpl->visibility = mutableFun->visibility;
				fnTmpl->isExternal = fun->isExternal || realClassDecl->isExternal;

				classType->methods[originalName] = fnTmpl;
				continue;
			}

			auto funType = std::make_shared<FunctionType>(mutableFun->name);
			funType->returnType = TypeResolver::Resolve(mutableFun->returnType.get(), m_context);
			if (funType->returnType == nullptr && !mutableFun->isExprBody)
			{
				funType->returnType = m_context.tUnit;
			}
			funType->isVararg = mutableFun->isVararg;
			for (const auto& [_, type] : mutableFun->parameters)
			{
				funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
			}

			if (funType->returnType == nullptr && mutableFun->isExprBody)
			{ // On-demand type evaluation for expression functions
				m_context.env.PushScope();

				for (std::size_t j = 0; j < mutableFun->parameters.size(); ++j)
				{
					m_context.env.Define(mutableFun->parameters[j].name, funType->paramTypes[j], false);
				}

				const auto prevFun = m_context.location.currentFunction;
				m_context.location.currentFunction = funType;

				if (mutableFun->body && !mutableFun->body->statements.empty())
				{
					if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(mutableFun->body->statements[0].get()))
					{
						funType->returnType = m_context.evaluateFunctionCallback(retStmt->expr.get());
					}
				}

				m_context.location.currentFunction = prevFun;
				m_context.env.PopScope();
			}

			funType->visibility = mutableFun->visibility;
			funType->isExternal = fun->isExternal || realClassDecl->isExternal;

			if (funType->isExternal)
			{
				m_context.externalFunctions.insert(funType->name);
			}

			bool existsInBase = classType->baseClass && classType->baseClass->methods.contains(originalName);

			if (mutableFun->isOverride && !existsInBase)
			{
				throw std::runtime_error("Semantic Error: Method '" + originalName + "' is marked 'override' but no matching method found in base class '" + classType->baseClass->name + "'");
			}
			if (!mutableFun->isOverride && existsInBase)
			{
				throw std::runtime_error("Semantic Error: Method '" + originalName + "' hides base class method. Add the 'override' modifier.");
			}

			classType->methods[originalName] = funType;
		}
		else if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
		{
			const auto mutableCtor = const_cast<ast::ConstructorDecl*>(ctor);

			ast::FunDecl::Parameter thisParam;
			thisParam.name = "this";
			auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
			thisTypeNode->name = uniqueName;
			thisParam.type = std::move(thisTypeNode);
			mutableCtor->parameters.insert(mutableCtor->parameters.begin(), std::move(thisParam));

			mutableCtor->name = uniqueName + "_" + uniqueName;
			m_context.allFunctionNames.insert(mutableCtor->name);

			auto funType = std::make_shared<FunctionType>(mutableCtor->name);
			funType->returnType = m_context.tUnit;
			funType->isVararg = mutableCtor->isVararg;
			for (const auto& [_, type] : mutableCtor->parameters)
			{
				funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), m_context));
			}
			funType->isExternal = realClassDecl->isExternal;

			classType->methods[uniqueName] = funType;
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