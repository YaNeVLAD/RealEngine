#include <IgniLang/Semantic/Helpers/Declaration/Function.hpp>

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>

namespace igni::sem::Declaration
{

std::shared_ptr<FunctionType> Function(const ast::FunDecl* decl, SemanticContext& ctx, const re::String& moduleName)
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

	auto funType = std::make_shared<FunctionType>(mangledName);
	funType->moduleName = moduleName;
	funType->visibility = decl->visibility;
	funType->isVararg = decl->isVararg;
	funType->isExternal = decl->isExternal;
	funType->isSuspend = decl->isSuspend;
	funType->paramTypes = std::move(paramTypes);
	funType->nativeTargetName = decl->name;
	funType->returnType = TypeResolver::Resolve(decl->returnType.get(), ctx);

	for (const auto& anno : decl->annotations)
	{ // FFI - external function name lookup
		if (ctx.ffiAnnotations.contains(anno.name) && anno.argument)
		{
			if (const auto lit = dynamic_cast<const ast::LiteralExpr*>(anno.argument.get()))
			{
				if (lit->token.type == TokenType::StringConst)
				{
					funType->nativeTargetName = lit->token.lexeme.substr(1, lit->token.lexeme.length() - 2);
				}
			}
		}
	}

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

	ctx.bindings.funMeta[decl] = FunMeta{ mangledName, false };

	return funType;
}

std::shared_ptr<FunctionType> Method(const ast::FunDecl* decl, const std::shared_ptr<ClassType>& classType, SemanticContext& ctx)
{
	re::String mangledName = classType->name + "_" + decl->name;
	std::vector<std::shared_ptr<SemanticType>> paramTypes;

	paramTypes.push_back(classType);

	for (const auto& [_, type] : decl->parameters)
	{
		auto pType = TypeResolver::Resolve(type.get(), ctx);
		paramTypes.push_back(pType);
		if (!decl->isExternal)
		{
			mangledName = mangledName + "@" + pType->name;
		}
	}

	auto funType = std::make_shared<FunctionType>(mangledName);
	funType->moduleName = classType->moduleName;
	funType->visibility = decl->visibility;
	funType->isVararg = decl->isVararg;
	funType->isExternal = decl->isExternal;
	funType->isSuspend = decl->isSuspend;
	funType->paramTypes = std::move(paramTypes);
	funType->returnType = TypeResolver::Resolve(decl->returnType.get(), ctx);

	for (const auto& anno : decl->annotations)
	{ // FFI - external function name lookup
		if (ctx.ffiAnnotations.contains(anno.name) && anno.argument)
		{
			if (const auto lit = dynamic_cast<const ast::LiteralExpr*>(anno.argument.get()))
			{
				if (lit->token.type == TokenType::StringConst)
				{
					funType->nativeTargetName = lit->token.lexeme.substr(1, lit->token.lexeme.length() - 2);
				}
			}
		}
	}

	if (funType->returnType == nullptr && decl->isExprBody)
	{
		ctx.env.PushScope();
		ctx.env.Define("this", classType, false);

		for (std::size_t i = 0; i < decl->parameters.size(); ++i)
		{
			ctx.env.Define(decl->parameters[i].name, funType->paramTypes[i + 1], false);
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

	ctx.bindings.funMeta[decl] = FunMeta{ mangledName, true, classType->name };

	return funType;
}

std::shared_ptr<FunctionType> Constructor(const ast::ConstructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx)
{
	re::String mangledName = classType->name + "_" + classType->name;
	std::vector<std::shared_ptr<SemanticType>> paramTypes;

	paramTypes.push_back(classType);

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
	funType->isSuspend = false; // TODO: Suspended constructors
	funType->isExternal = isClassExternal || decl->isExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;
	funType->paramTypes = std::move(paramTypes);

	ctx.allFunctionNames.insert(mangledName);
	ctx.instantiatedFunctions[mangledName] = funType;

	if (funType->isExternal)
	{
		ctx.externalFunctions.insert(mangledName);
	}

	ctx.bindings.funMeta[decl] = FunMeta{ mangledName, true, classType->name };

	return funType;
}

std::shared_ptr<FunctionType> Destructor(const ast::DestructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx)
{
	auto mangledName = classType->name + "_destructor";
	ctx.allFunctionNames.insert(mangledName);

	auto funType = std::make_shared<FunctionType>(mangledName);
	funType->returnType = ctx.tUnit;
	funType->isSuspend = false;
	funType->isExternal = isClassExternal || decl->isExternal;
	funType->visibility = decl->visibility;
	funType->moduleName = classType->moduleName;
	funType->paramTypes.push_back(classType);

	ctx.instantiatedFunctions[mangledName] = funType;

	if (funType->isExternal)
	{
		ctx.externalFunctions.insert(mangledName);
	}

	ctx.bindings.funMeta[decl] = FunMeta{ mangledName, true, classType->name };

	return funType;
}

std::shared_ptr<GenericFunctionTemplate> GenericFunction(const ast::FunDecl* decl, const re::String& moduleName, const std::shared_ptr<ClassType>& parentClass)
{
	auto tmpl = std::make_shared<GenericFunctionTemplate>(decl->name);
	tmpl->astNode = decl;
	tmpl->typeParams = decl->typeParams;
	tmpl->moduleName = moduleName;
	tmpl->visibility = decl->visibility;
	tmpl->isExternal = decl->isExternal;

	tmpl->isMethod = parentClass != nullptr;
	tmpl->parentClass = parentClass;

	return tmpl;
}

} // namespace igni::sem::Declaration