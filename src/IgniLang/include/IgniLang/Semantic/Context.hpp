#pragma once

#include <IgniLang/BindingContext.hpp>
#include <IgniLang/Semantic/Enviroment.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <functional>

namespace igni::sem
{

struct TraversalContext
{
	std::shared_ptr<ClassType> currentClass = nullptr;
	std::shared_ptr<FunctionType> currentFunction = nullptr;
	std::shared_ptr<SemanticType> expectedReturnType = nullptr;
	std::shared_ptr<SemanticType> currentReturnType = nullptr;
	std::shared_ptr<ModuleType> currentPackage = nullptr;
	bool isInsideLaunch = false;
};

struct LambdaMeta
{
	re::String mangledName;
};

struct SemanticContext
{
	Environment env;
	TraversalContext location;
	BindingContext bindings;

	std::unordered_map<re::String, re::String> importAliases;

	std::unordered_map<re::String, std::shared_ptr<ClassType>> allClassTypes;

	std::unordered_set<re::String> allFunctionNames;
	std::unordered_set<re::String> externalFunctions;

	std::vector<std::unique_ptr<ast::Statement>> pendingStatements;
	std::vector<ast::FunDecl*> m_pendingFunInstantiations;
	std::vector<ast::ClassDecl*> m_pendingClassInstantiations;

	std::unordered_set<const ast::Node*> m_instantiatedNodes;
	std::unordered_map<re::String, std::shared_ptr<ClassType>> instantiatedClasses;
	std::unordered_map<re::String, std::shared_ptr<FunctionType>> instantiatedFunctions;

	std::size_t lambdaCounter = 0;
	std::vector<const ast::LambdaExpr*> allLambdas;

	std::shared_ptr<ClassType> tInt;
	std::shared_ptr<ClassType> tDouble;
	std::shared_ptr<ClassType> tBool;
	std::shared_ptr<ClassType> tString;
	std::shared_ptr<ClassType> tAny;
	std::shared_ptr<ClassType> tType = std::make_shared<ClassType>("Type");

	std::shared_ptr<PrimitiveType> tUnit = std::make_shared<PrimitiveType>("Unit");
	std::shared_ptr<PrimitiveType> tNull = std::make_shared<PrimitiveType>("Null");

	std::function<std::shared_ptr<ClassType>(
		std::shared_ptr<GenericClassTemplate>,
		const std::vector<std::shared_ptr<SemanticType>>&)>
		instantiateClassCallback;

	std::function<std::shared_ptr<FunctionType>(
		const std::shared_ptr<GenericFunctionTemplate>&,
		const std::vector<std::shared_ptr<SemanticType>>&)>
		instantiateFunctionCallback;

	std::function<std::shared_ptr<SemanticType>(const ast::Expr* expr)> evaluateFunctionCallback;
};
} // namespace igni::sem