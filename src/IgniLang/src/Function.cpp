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
	re::String mangledName = decl->name;
	std::vector<std::shared_ptr<SemanticType>> paramTypes;

	for (const auto& [_, type] : decl->parameters)
	{
		auto pType = TypeResolver::Resolve(type.get(), ctx);
		paramTypes.push_back(pType);

		if (!decl->isExternal)
		{
			mangledName = mangledName + "@" + pType->name;
		}
	}

	decl->name = mangledName;

	auto funType = std::make_shared<FunctionType>(mangledName);
	funType->moduleName = moduleName;
	funType->visibility = decl->visibility;
	funType->isVararg = decl->isVararg;
	funType->isExternal = decl->isExternal;
	funType->paramTypes = std::move(paramTypes);
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

	ctx.instantiatedFunctions[funType->name] = funType;

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

	re::String mangledName = classType->name + "_" + classType->name;
	std::vector<std::shared_ptr<SemanticType>> paramTypes;

	for (const auto& [_, type] : decl->parameters)
	{
		auto pType = TypeResolver::Resolve(type.get(), ctx);
		paramTypes.push_back(pType);

		if (!isClassExternal && !decl->isExternal)
		{
			mangledName = mangledName + "@" + pType->name;
		}
	}

	auto funType = std::make_shared<FunctionType>(mangledName);
	funType->returnType = ctx.tUnit;
	funType->isVararg = decl->isVararg;
	funType->isExternal = isClassExternal || decl->isExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;
	funType->paramTypes = std::move(paramTypes);

	decl->name = mangledName;
	ctx.allFunctionNames.insert(mangledName);

	ctx.instantiatedFunctions[mangledName] = funType;

	return funType;
}

std::shared_ptr<FunctionType> Destructor(ast::DestructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx)
{
	decl->name = classType->name + "_destructor";
	ctx.allFunctionNames.insert(decl->name);

	auto funType = std::make_shared<FunctionType>(decl->name);
	funType->returnType = ctx.tUnit;
	funType->isExternal = isClassExternal || decl->isExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;

	const auto thisTypeNode = std::make_unique<ast::SimpleTypeNode>();
	thisTypeNode->name = classType->name;
	funType->paramTypes.emplace_back(TypeResolver::Resolve(thisTypeNode.get(), ctx));

	ctx.instantiatedFunctions[decl->name] = funType;

	return funType;
}

std::shared_ptr<GenericFunctionTemplate> GenericFunction(ast::FunDecl* decl, const re::String& moduleName)
{
	auto tmpl = std::make_shared<GenericFunctionTemplate>(decl->name);
	tmpl->astNode = decl;
	tmpl->typeParams = decl->typeParams;
	tmpl->moduleName = moduleName;
	tmpl->visibility = decl->visibility;
	tmpl->isExternal = decl->isExternal;
	return tmpl;
}

} // namespace igni::sem::Declaration