#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>

namespace igni::sem::Declaration
{

re::String InjectThisKeyword(ast::FunDecl* decl, const re::String& className)
{
	const re::String originalName = decl->name;
	if (decl->parameters.empty() || decl->parameters[0].name != "this")
	{
		ast::FunDecl::Parameter thisParam;
		thisParam.name = "this";
		auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
		thisTypeNode->name = className;
		thisParam.type = std::move(thisTypeNode);
		decl->parameters.insert(decl->parameters.begin(), std::move(thisParam));
	}
	decl->name = className + "_" + originalName;

	return originalName;
}

std::shared_ptr<FunctionType> Function(ast::FunDecl* decl, SemanticContext& ctx, const re::String& moduleName)
{
	auto funType = std::make_shared<FunctionType>(decl->name);
	funType->moduleName = moduleName;
	funType->visibility = decl->visibility;
	funType->isVararg = decl->isVararg;
	funType->isExternal = decl->isExternal;

	for (const auto& param : decl->parameters)
	{
		funType->paramTypes.push_back(TypeResolver::Resolve(param.type.get(), ctx));
	}

	funType->returnType = TypeResolver::Resolve(decl->returnType.get(), ctx);

	if (funType->returnType == nullptr && decl->isExprBody)
	{ // On-demand type evaluation for expression functions
		ctx.env.PushScope();
		for (std::size_t i = 0; i < decl->parameters.size(); ++i)
		{
			ctx.env.Define(decl->parameters[i].name, funType->paramTypes[i], false);
		}

		const auto prevFun = ctx.location.currentFunction;
		ctx.location.currentFunction = funType;

		if (decl->body && !decl->body->statements.empty())
		{
			if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(decl->body->statements[0].get()))
			{
				funType->returnType = ctx.evaluateFunctionCallback(retStmt->expr.get());
			}
		}

		ctx.location.currentFunction = prevFun;
		ctx.env.PopScope();
	}
	else if (funType->returnType == nullptr && !decl->isExprBody)
	{
		funType->returnType = ctx.tUnit;
	}

	if (funType->isExternal)
	{
		ctx.externalFunctions.insert(funType->name);
	}
	ctx.allFunctionNames.insert(funType->name);

	return funType;
}

std::shared_ptr<FunctionType> Method(ast::FunDecl* decl, const re::String& originalName, const std::shared_ptr<ClassType>& classType, SemanticContext& ctx)
{
	auto funType = Function(decl, ctx, classType->moduleName);

	const bool existsInBase = classType->baseClass && classType->baseClass->methods.contains(originalName);
	if (decl->isOverride && !existsInBase)
	{
		IGNI_SEM_ERR(decl, "Method '" + originalName + "' is marked 'override' but no matching method found in base class");
	}
	if (!decl->isOverride && existsInBase)
	{
		IGNI_SEM_ERR(decl, "Method '" + originalName + "' hides base class method. Add the 'override' modifier.");
	}

	classType->methods[originalName] = funType;
	return funType;
}

std::shared_ptr<FunctionType> Constructor(ast::ConstructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx)
{
	ast::FunDecl::Parameter thisParam;
	thisParam.name = "this";
	auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
	thisTypeNode->name = classType->name;
	thisParam.type = std::move(thisTypeNode);
	decl->parameters.insert(decl->parameters.begin(), std::move(thisParam));

	decl->name = classType->name + "_" + classType->name;
	ctx.allFunctionNames.insert(decl->name);

	auto funType = std::make_shared<FunctionType>(decl->name);
	funType->returnType = ctx.tUnit;
	funType->isVararg = decl->isVararg;
	funType->isExternal = isClassExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;

	for (const auto& [_, type] : decl->parameters)
	{
		funType->paramTypes.emplace_back(TypeResolver::Resolve(type.get(), ctx));
	}

	classType->methods[classType->name] = funType;

	return funType;
}

std::shared_ptr<FunctionType> Destructor(ast::DestructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx)
{
	decl->name = classType->name + "_destructor";
	ctx.allFunctionNames.insert(decl->name);

	auto funType = std::make_shared<FunctionType>(decl->name);
	funType->returnType = ctx.tUnit;
	funType->isExternal = isClassExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;

	const auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
	thisTypeNode->name = classType->name;
	funType->paramTypes.emplace_back(TypeResolver::Resolve(thisTypeNode.get(), ctx));

	classType->methods["~" + classType->name] = funType;

	return funType;
}

} // namespace igni::sem::Declaration