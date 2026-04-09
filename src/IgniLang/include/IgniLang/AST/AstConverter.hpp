#pragma once

#include <Core/HashedString.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/CST/CstNode.hpp>

#include <iostream>
#include <stdexcept>

namespace igni
{

using namespace re::literals;

class AstConverter
{
public:
	std::unique_ptr<ast::Program> Convert(const CstNode* root)
	{
		if (!root || root->symbol.Hashed() != "Program"_hs)
		{
			return nullptr;
		}

		auto program = std::make_unique<ast::Program>();

		// Program -> OptPackageDecl ImportDeclList TopLevelDeclList
		const auto& optPkg = root->children[0];
		const auto& importsList = root->children[1];
		const auto& decls = root->children[2];

		if (!optPkg->children.empty() && optPkg->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
		{
			// PackageDecl -> package PackagePath ;
			program->packageName = ExtractPathString(optPkg->children[0]->children[1].get());
		}

		ExtractImports(importsList.get(), program->imports);

		FlattenTopLevelDecls(decls.get(), program->statements);

		return program;
	}

private:
	static ast::Visibility ParseVisibility(const CstNode* optVisNode)
	{
		if (optVisNode->children.empty() || optVisNode->children[0]->symbol.Hashed() == "<EPSILON>"_hs)
		{
			return ast::Visibility::Public;
		}

		const auto hashed = optVisNode->children[0]->symbol.Hashed();
		if (hashed == "private"_hs)
		{
			return ast::Visibility::Private;
		}
		if (hashed == "internal"_hs)
		{
			return ast::Visibility::Internal;
		}

		return ast::Visibility::Public;
	}

	static re::String ExtractPathString(const CstNode* pathNode)
	{
		// Path -> ident
		if (pathNode->children.size() == 1)
		{
			return pathNode->children[0]->token->lexeme;
		}
		// Path -> Path . ident
		if (pathNode->children.size() == 3)
		{
			const re::String leftPath = ExtractPathString(pathNode->children[0].get());
			const re::String rightIdent = pathNode->children[2]->token->lexeme;
			return leftPath + '.' + rightIdent;
		}

		return re::String{};
	}

	static void ExtractImports(const CstNode* listNode, std::vector<std::unique_ptr<ast::ImportDecl>>& outImports)
	{
		if (!listNode || listNode->symbol.Hashed() == "<EPSILON>"_hs || listNode->children.empty())
		{
			return;
		}

		// ImportDeclList -> ImportDeclList ImportDecl
		if (listNode->children.size() == 2)
		{
			ExtractImports(listNode->children[0].get(), outImports);

			const auto& declNode = listNode->children[1];

			auto importDecl = std::make_unique<ast::ImportDecl>();

			// declNode->children[0] = "import"
			// declNode->children[1] = ImportPath
			importDecl->path = ExtractPathString(declNode->children[1].get());

			// declNode->children[2] = OptDotStar
			const auto& optStar = declNode->children[2];
			importDecl->isStar = (!optStar->children.empty() && optStar->children[0]->symbol.Hashed() == "."_hs);

			outImports.push_back(std::move(importDecl));
		}
	}

	static void FlattenTopLevelDecls(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol.Hashed() == "<EPSILON>"_hs || listNode->children.empty())
		{
			return;
		}

		if (listNode->children.size() == 2)
		{
			FlattenTopLevelDecls(listNode->children[0].get(), outStmts);

			if (auto decl = ConvertTopLevelDecl(listNode->children[1].get()))
			{
				outStmts.push_back(std::move(decl));
			}
		}
	}

	static std::unique_ptr<ast::Decl> ConvertTopLevelDecl(const CstNode* topLevelDecl)
	{
		// TopLevelDecl -> AnnotationList [0] OptVisibility [1] Decl [2]
		const auto& optVisNode = topLevelDecl->children[1];
		const auto& declNode = topLevelDecl->children[2];
		const auto& actualDecl = declNode->children[0];

		if (auto decl = ConvertActualDecl(actualDecl.get()))
		{
			decl->visibility = ParseVisibility(optVisNode.get());

			return decl;
		}

		return nullptr;
	}

	static std::unique_ptr<ast::Decl> ConvertActualDecl(const CstNode* actualDecl)
	{
		switch (actualDecl->symbol.Hashed())
		{
		case "ValDecl"_hs:
		case "VarDecl"_hs: {
			// VarDecl/ValDecl -> OptMod [0] var/val [1] ident [2] OptColonType [3] OptInit [4] ; [5]
			bool isExternal = false;
			if (const auto& optMod = actualDecl->children[0];
				!optMod->children.empty() && optMod->children[0]->symbol.Hashed() == "external"_hs)
			{
				isExternal = true;
			}

			const re::String name = actualDecl->children[2]->token->lexeme;

			std::unique_ptr<ast::TypeNode> explicitType = nullptr;
			if (const auto& optColon = actualDecl->children[3]; optColon->children.size() == 2)
			{
				explicitType = ConvertType(optColon->children[1].get());
			}

			std::unique_ptr<ast::Expr> initializerExpr = nullptr;
			if (const auto& optInit = actualDecl->children[4];
				!optInit->children.empty() && optInit->children[0]->symbol.Hashed() == "="_hs)
			{
				initializerExpr = ConvertExpr(optInit->children[1].get());
			}

			if (actualDecl->symbol.Hashed() == "ValDecl"_hs)
			{
				auto val = std::make_unique<ast::ValDecl>();
				val->name = name;
				val->type = std::move(explicitType);
				val->initializer = std::move(initializerExpr);
				val->isExternal = isExternal;

				return val;
			}

			auto var = std::make_unique<ast::VarDecl>();
			var->name = name;
			var->type = std::move(explicitType);
			var->initializer = std::move(initializerExpr);
			var->isExternal = isExternal;

			return var;
		}
		case "FunDecl"_hs: { // FunDecl -> OptFunMod [0] fun [1] OptTypeParams [2] FunName [3] ( [4] OptFormPars [5] ) [6] OptColonType [7] FunBody [8]
			auto funDecl = std::make_unique<ast::FunDecl>();

			if (const auto& optFunMod = actualDecl->children[0];
				!optFunMod->children.empty() && optFunMod->children[0]->symbol.Hashed() == "external"_hs)
			{ // External modifier [0]
				funDecl->isExternal = true;
			}

			if (const auto& funNameNode = actualDecl->children[3]; funNameNode->children.size() == 1)
			{ // FunName [3]
				funDecl->name = funNameNode->children[0]->token->lexeme;
			}
			else
			{
				funDecl->name = funNameNode->children.back()->token->lexeme;
			}

			ExtractTypeParams(actualDecl->children[2].get(), funDecl->typeParams);

			if (const auto& optColon = actualDecl->children[7]; optColon->children.size() == 2)
			{ // Return type [7]
				funDecl->returnType = ConvertType(optColon->children[1].get());
			}

			// Parameters [5]
			ExtractParameters(actualDecl->children[5].get(), funDecl.get());

			if (const auto& funBody = actualDecl->children[8]; funBody->children[0]->symbol.Hashed() == "Block"_hs)
			{ // Body [8]
				funDecl->body = ConvertBlock(funBody->children[0].get());
			}
			else if (funBody->children[0]->symbol.Hashed() == "="_hs)
			{ // Desugaring: fun ident(args) = expr; => fun ident(args) { return expr; }
				funDecl->isExprBody = true;

				auto expr = ConvertExpr(funBody->children[1].get());

				auto returnStatement = std::make_unique<ast::ReturnStmt>();
				returnStatement->expr = std::move(expr);

				funDecl->body = std::make_unique<ast::Block>();
				funDecl->body->statements.push_back(std::move(returnStatement));
			}

			return funDecl;
		}
		case "ClassDecl"_hs: {
			// ClassDecl -> OptClassMod [0] class [1] OptTypeParams [2] ident [3] OptPrimaryCtor [4] OptBaseClass [5] { [6] ClassMemberList [7] } [8]
			// OptBaseClass -> : Type [0] ( [1] OptActPars [2] ) [3] | : [0] Type [1] | \e
			auto classDecl = std::make_unique<ast::ClassDecl>();

			if (const auto& optClassMod = actualDecl->children[0];
				!optClassMod->children.empty() && optClassMod->children[0]->symbol.Hashed() == "external"_hs)
			{
				classDecl->isExternal = true;
			}

			classDecl->name = actualDecl->children[3]->token->lexeme;

			if (const auto& optBaseClass = actualDecl->children[5];
				!optBaseClass->children.empty() && optBaseClass->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
			{
				classDecl->baseClass = std::make_unique<ast::BaseClassInit>();

				// OptBaseClass -> : [0] Type [1]
				classDecl->baseClass->type = ConvertType(optBaseClass->children[1].get());

				// OptBaseClass -> : [0] Type [1] ( [2] OptActPars [3] ) [4]
				if (optBaseClass->children.size() == 5)
				{
					ExtractArguments(optBaseClass->children[3].get(), classDecl->baseClass->arguments);
				}
			}

			ExtractTypeParams(actualDecl->children[2].get(), classDecl->typeParams);

			const auto& optPrimaryCtor = actualDecl->children[4];
			const auto& classMemberList = actualDecl->children[7];

			std::unique_ptr<ast::ConstructorDecl> primaryCtor = nullptr;

			if (!optPrimaryCtor->children.empty() && optPrimaryCtor->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
			{
				primaryCtor = std::make_unique<ast::ConstructorDecl>();
				primaryCtor->name = classDecl->name;
				primaryCtor->body = std::make_unique<ast::Block>();

				ExtractPrimaryCtorParams(optPrimaryCtor->children[1].get(), classDecl.get(), primaryCtor.get());
			}
			else if (classDecl->baseClass && !classDecl->baseClass->arguments.empty())
			{ // Empty parent constructor
				primaryCtor = std::make_unique<ast::ConstructorDecl>();
				primaryCtor->name = classDecl->name;
				primaryCtor->body = std::make_unique<ast::Block>();
			}

			if (primaryCtor && classDecl->baseClass && !classDecl->baseClass->arguments.empty())
			{
				auto superCall = std::make_unique<ast::CallExpr>();
				auto superId = std::make_unique<ast::IdentifierExpr>();
				superId->name = "super";
				superCall->callee = std::move(superId);

				for (const auto& arg : classDecl->baseClass->arguments)
				{
					superCall->arguments.push_back(arg->CloneExpr());
				}

				auto exprStmt = std::make_unique<ast::ExprStmt>();
				exprStmt->expr = std::move(superCall);

				primaryCtor->body->statements.insert(primaryCtor->body->statements.begin(), std::move(exprStmt));
			}

			ExtractClassMembers(classMemberList.get(), classDecl->members);

			if (primaryCtor)
			{
				classDecl->members.push_back(std::move(primaryCtor));
			}

			return classDecl;
		}
		case "ConstructorDecl"_hs: { // ConstructorDecl -> ident [0] ( [1] OptFormPars [2] ) [3] Block [4]
			auto ctorDecl = std::make_unique<ast::ConstructorDecl>();

			ctorDecl->name = actualDecl->children[0]->token->lexeme;

			// Reuse FunDecl for temporary storage
			ast::FunDecl tempFun;
			ExtractParameters(actualDecl->children[2].get(), &tempFun);

			ctorDecl->parameters = std::move(tempFun.parameters);
			ctorDecl->isVararg = tempFun.isVararg;

			const auto& bodyNode = actualDecl->children[4];
			ctorDecl->body = ConvertBlock(bodyNode.get());

			return ctorDecl;
		}
		case "DestructorDecl"_hs: {
			// DestructorDecl -> ~ ident ( ) Block
			auto dtorDecl = std::make_unique<ast::DestructorDecl>();

			dtorDecl->name = actualDecl->children[1]->token->lexeme;

			const auto& bodyNode = actualDecl->children[4];
			dtorDecl->body = ConvertBlock(bodyNode.get());

			return dtorDecl;
		}
		default:
			return nullptr;
		}
	}

	static std::unique_ptr<ast::Block> ConvertBlock(const CstNode* blockNode)
	{
		auto block = std::make_unique<ast::Block>();
		FlattenStatements(blockNode->children[1].get(), block->statements);
		return block;
	}

	static void FlattenStatements(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol.Hashed() == "<EPSILON>"_hs || listNode->children.empty())
		{
			return;
		}

		if (listNode->children.size() == 2)
		{
			FlattenStatements(listNode->children[0].get(), outStmts);

			const auto& stmtNode = listNode->children[1];
			const auto& actualStmt = stmtNode->children[0];

			switch (actualStmt->symbol.Hashed())
			{
			case "ValDecl"_hs:
			case "VarDecl"_hs:
			case "FunDecl"_hs:
				outStmts.push_back(ConvertActualDecl(actualStmt.get()));
				break;
			case "return"_hs: {
				auto ret = std::make_unique<ast::ReturnStmt>();
				if (stmtNode->children[1]->symbol.Hashed() != "<EPSILON>"_hs)
				{
					ret->expr = ConvertExpr(stmtNode->children[1]->children[0].get());
				}
				outStmts.push_back(std::move(ret));
				break;
			}
			case "Expr"_hs: {
				auto exprStmt = std::make_unique<ast::ExprStmt>();
				exprStmt->expr = ConvertExpr(actualStmt.get());
				outStmts.push_back(std::move(exprStmt));
				break;
			}
			case "IfStmt"_hs:
				outStmts.push_back(ConvertIfStmt(actualStmt.get()));
				break;
			case "WhileStmt"_hs:
				outStmts.push_back(ConvertWhileStmt(actualStmt.get()));
				break;
			case "ForStmt"_hs:
				outStmts.push_back(ConvertForStmt(actualStmt.get()));
				break;
			case "Block"_hs: {
				outStmts.push_back(ConvertBlock(actualStmt.get()));
				break;
			}
			default:
				throw std::invalid_argument("Invalid statement");
			}
		}
	}

	static std::unique_ptr<ast::Expr> ConvertExpr(const CstNode* exprNode)
	{
		if (!exprNode)
		{
			return nullptr;
		}

		switch (exprNode->symbol.Hashed())
		{
		case "Designator"_hs: {
			// Designator -> ident [0] OptExprTypeArgs [1]
			if (exprNode->children.size() == 2 && exprNode->children[0]->symbol.Hashed() == "ident"_hs)
			{
				auto id = std::make_unique<ast::IdentifierExpr>();
				id->name = exprNode->children[0]->token->lexeme;

				if (const auto& optTypeArgs = exprNode->children[1];
					!optTypeArgs->children.empty() && optTypeArgs->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
				{
					ExtractTypeList(optTypeArgs->children[2].get(), id->typeArgs);
				}

				return id;
			}
			// Designator -> Designator [0] ( [1] OptActPars [2] ) [3]
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol.Hashed() == "("_hs)
			{
				auto call = std::make_unique<ast::CallExpr>();
				call->callee = ConvertExpr(exprNode->children[0].get());
				ExtractArguments(exprNode->children[2].get(), call->arguments);

				return call;
			}
			// Designator -> Designator [0] [ [1] Expr [2] ] [3]
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol.Hashed() == "["_hs)
			{
				auto idx = std::make_unique<ast::IndexExpr>();
				idx->array = ConvertExpr(exprNode->children[0].get());
				idx->index = ConvertExpr(exprNode->children[2].get());

				return idx;
			}
			// Designator -> Designator [0] . [1] ident [2] OptExprTypeArgs [3]
			// Designator -> Designator [0] ?. [1] ident [2] OptExprTypeArgs [3]
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol.Hashed() == "."_hs)
			{
				auto memberAccess = std::make_unique<ast::MemberAccessExpr>();
				memberAccess->object = ConvertExpr(exprNode->children[0].get());
				memberAccess->member = exprNode->children[2]->token->lexeme;

				if (const auto& optTypeArgs = exprNode->children[1];
					!optTypeArgs->children.empty() && optTypeArgs->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
				{
					ExtractTypeList(optTypeArgs->children[2].get(), memberAccess->typeArgs);
				}

				return memberAccess;
			}

			break;
		}
		case "PrimaryExpr"_hs: {
			const auto& child = exprNode->children[0];
			switch (child->symbol.Hashed())
			{
			case "true"_hs:
			case "false"_hs:
			case "intCon"_hs:
			case "floatCon"_hs:
			case "stringCon"_hs: {
				auto lit = std::make_unique<ast::LiteralExpr>();
				lit->token = *child->token;
				return lit;
			}
			case "("_hs:
				return ConvertExpr(exprNode->children[1].get());
			case "Designator"_hs:
				return ConvertExpr(child.get());
			default:
				throw std::invalid_argument("Invalid primary expression");
			}
		}
		case "PostfixExpr"_hs: {
			if (exprNode->children.size() == 2)
			{
				auto unExpr = std::make_unique<ast::UnaryExpr>();
				unExpr->operand = ConvertExpr(exprNode->children[0].get());
				unExpr->op = exprNode->children[1]->symbol;
				unExpr->isPostfix = true;
				return unExpr;
			}
			break;
		}
		case "UnaryExpr"_hs: {
			if (exprNode->children.size() == 2)
			{
				auto unExpr = std::make_unique<ast::UnaryExpr>();
				unExpr->op = exprNode->children[0]->symbol;
				unExpr->operand = ConvertExpr(exprNode->children[1].get());
				unExpr->isPostfix = false;
				return unExpr;
			}
			break;
		}
		default:
			break;
		}

		if (exprNode->children.size() == 1)
		{
			if (exprNode->children[0]->symbol.Hashed() == "<EPSILON>"_hs)
			{
				return nullptr;
			}
			return ConvertExpr(exprNode->children[0].get());
		}

		if (exprNode->children.size() == 3)
		{
			if (exprNode->children[1]->symbol.Hashed() == "="_hs)
			{
				auto assign = std::make_unique<ast::AssignExpr>();
				assign->target = ConvertExpr(exprNode->children[0].get());
				assign->value = ConvertExpr(exprNode->children[2].get());
				return assign;
			}

			auto binExpr = std::make_unique<ast::BinaryExpr>();
			binExpr->left = ConvertExpr(exprNode->children[0].get());
			binExpr->op = exprNode->children[1]->symbol;
			binExpr->right = ConvertExpr(exprNode->children[2].get());

			return binExpr;
		}

		std::cerr << "Unhandled Expr node: " << exprNode->symbol << "\n";
		return nullptr;
	}

	static std::unique_ptr<ast::IfStmt> ConvertIfStmt(const CstNode* ifNode)
	{
		auto ifStmt = std::make_unique<ast::IfStmt>();
		ifStmt->condition = ConvertExpr(ifNode->children[2].get());
		ifStmt->thenBranch = ConvertBlock(ifNode->children[4].get());

		if (const auto& optElse = ifNode->children[5]; optElse->children.size() == 2)
		{
			const auto& elseBody = optElse->children[1]->children[0];
			switch (elseBody->symbol.Hashed())
			{
			case "IfStmt"_hs:
				ifStmt->elseBranch = ConvertIfStmt(elseBody.get());
				break;
			case "Block"_hs:
				ifStmt->elseBranch = ConvertBlock(elseBody.get());
				break;
			default:
				throw std::runtime_error("Unhandled IfStmt");
			}
		}

		return ifStmt;
	}

	static std::unique_ptr<ast::WhileStmt> ConvertWhileStmt(const CstNode* whileNode)
	{
		auto stmt = std::make_unique<ast::WhileStmt>();
		stmt->condition = ConvertExpr(whileNode->children[2].get());
		stmt->body = ConvertBlock(whileNode->children[4].get());

		return stmt;
	}

	static std::unique_ptr<ast::ForStmt> ConvertForStmt(const CstNode* forNode)
	{
		auto stmt = std::make_unique<ast::ForStmt>();
		stmt->iteratorName = forNode->children[2]->token->lexeme;
		stmt->startExpr = ConvertExpr(forNode->children[4].get());
		stmt->endExpr = ConvertExpr(forNode->children[6].get());
		stmt->body = ConvertBlock(forNode->children[8].get());

		return stmt;
	}

	static std::unique_ptr<ast::TypeNode> ConvertType(const CstNode* typeNode)
	{
		if (!typeNode)
		{
			return nullptr;
		}

		switch (typeNode->children[0]->symbol.Hashed())
		{
		case "ident"_hs: {
			auto simpleType = std::make_unique<ast::SimpleTypeNode>();
			simpleType->name = typeNode->children[0]->token->lexeme;

			if (const auto& optTypeArgs = typeNode->children[1]; optTypeArgs->children.size() == 3)
				ExtractTypeList(optTypeArgs->children[1].get(), simpleType->typeArgs);

			simpleType->isNullable = (!typeNode->children[2]->children.empty() && typeNode->children[2]->children[0]->symbol.Hashed() == "?"_hs);
			return simpleType;
		}
		case "("_hs: {
			auto funType = std::make_unique<ast::FunctionTypeNode>();

			if (const auto& optTypeList = typeNode->children[1]; optTypeList->children.size() == 1 && optTypeList->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
			{
				ExtractTypeList(optTypeList->children[0].get(), funType->paramTypes);
			}

			funType->returnType = ConvertType(typeNode->children[4].get());
			funType->isNullable = (!typeNode->children[5]->children.empty() && typeNode->children[5]->children[0]->symbol.Hashed() == "?"_hs);
			return funType;
		}
		default:
			throw std::runtime_error("Unhandled Type node: " + typeNode->symbol.ToString());
		}
	}

	static void ExtractTypeList(const CstNode* typeList, std::vector<std::unique_ptr<ast::TypeNode>>& outTypes)
	{
		if (typeList->children.size() == 3)
		{
			ExtractTypeList(typeList->children[0].get(), outTypes);
			outTypes.push_back(ConvertType(typeList->children[2].get()));
		}
		else if (typeList->children.size() == 1)
		{
			outTypes.push_back(ConvertType(typeList->children[0].get()));
		}
	}

	static void ExtractTypeParams(const CstNode* optTypeParams, std::vector<ast::GenericTypeParam>& outParams)
	{
		// OptTypeParams -> < TypeParams > | \e
		if (!optTypeParams || optTypeParams->children.empty() || optTypeParams->children[0]->symbol.Hashed() == "<EPSILON>"_hs)
		{
			return;
		}

		ExtractTypeParamList(optTypeParams->children[1].get(), outParams);
	}

	static void ExtractTypeParamList(const CstNode* typeParamsNode, std::vector<ast::GenericTypeParam>& outParams)
	{
		// TypeParams -> TypeParams , TypeParam | TypeParam
		if (typeParamsNode->children.size() == 3)
		{
			ExtractTypeParamList(typeParamsNode->children[0].get(), outParams);
			outParams.push_back(ParseTypeParam(typeParamsNode->children[2].get()));
		}
		else if (typeParamsNode->children.size() == 1)
		{
			outParams.push_back(ParseTypeParam(typeParamsNode->children[0].get()));
		}
	}

	static ast::GenericTypeParam ParseTypeParam(const CstNode* typeParamNode)
	{
		// TypeParam -> ident OptColonType
		ast::GenericTypeParam param;
		param.name = typeParamNode->children[0]->token->lexeme;

		if (const auto& optColon = typeParamNode->children[1]; optColon->children.size() == 2)
		{ // OptColonType -> : Type
			param.boundType = ConvertType(optColon->children[1].get());
		}
		else
		{
			param.boundType = nullptr;
		}

		return param;
	}

	static ast::FunDecl::Parameter ParseFormPar(const CstNode* node)
	{
		// FormPar -> ident : Type
		ast::FunDecl::Parameter p;
		p.name = node->children[0]->token->lexeme;
		p.type = ConvertType(node->children[2].get());
		return p;
	}

	static ast::FunDecl::Parameter ParseVarargPar(const CstNode* node)
	{
		// VarargPar -> ident : Type ...
		ast::FunDecl::Parameter p;
		p.name = node->children[0]->token->lexeme;
		p.type = ConvertType(node->children[2].get());
		return p;
	}

	static void ExtractNormalPars(const CstNode* node, std::vector<ast::FunDecl::Parameter>& outParams)
	{
		if (node->children.size() != 1)
		{
			if (node->children.size() == 3)
			{ // NormalPars , FormPar
				ExtractNormalPars(node->children[0].get(), outParams);
				outParams.emplace_back(ParseFormPar(node->children[2].get()));
			}
		}
		else
		{ // FormPar
			outParams.emplace_back(ParseFormPar(node->children[0].get()));
		}
	}

	static void ExtractParameters(const CstNode* optFormPars, ast::FunDecl* funDecl)
	{
		if (optFormPars->children.size() == 1 && optFormPars->children[0]->symbol.Hashed() == "<EPSILON>"_hs)
		{
			return;
		}

		if (const auto& formPars = optFormPars->children[0]; formPars->children.size() == 1)
		{
			if (formPars->children[0]->symbol.Hashed() == "NormalPars"_hs)
			{
				ExtractNormalPars(formPars->children[0].get(), funDecl->parameters);
			}
			else
			{ // VarargPar
				funDecl->parameters.push_back(ParseVarargPar(formPars->children[0].get()));
				funDecl->isVararg = true;
			}
		}
		else if (formPars->children.size() == 3)
		{ // NormalPars , VarargPar
			ExtractNormalPars(formPars->children[0].get(), funDecl->parameters);
			funDecl->parameters.push_back(ParseVarargPar(formPars->children[2].get()));
			funDecl->isVararg = true;
		}
	}

	static void ExtractArguments(const CstNode* optActPars, std::vector<std::unique_ptr<ast::Expr>>& outArgs)
	{
		if (optActPars->children.size() == 1 && optActPars->children[0]->symbol.Hashed() == "<EPSILON>"_hs)
		{
			return;
		}
		FlattenActPars(optActPars->children[0].get(), outArgs);
	}

	static void FlattenActPars(const CstNode* actPars, std::vector<std::unique_ptr<ast::Expr>>& outArgs)
	{
		if (actPars->children.size() == 3)
		{
			FlattenActPars(actPars->children[0].get(), outArgs);
			outArgs.push_back(ConvertExpr(actPars->children[2].get()));
		}
		else if (actPars->children.size() == 1)
		{
			outArgs.push_back(ConvertExpr(actPars->children[0].get()));
		}
	}

	static void ExtractClassMembers(const CstNode* listNode, std::vector<std::unique_ptr<ast::Decl>>& outMembers)
	{
		if (!listNode || listNode->symbol.Hashed() == "<EPSILON>"_hs || listNode->children.empty())
		{
			return;
		}

		// ClassMemberList -> ClassMemberList ClassMember
		if (listNode->children.size() == 2)
		{
			ExtractClassMembers(listNode->children[0].get(), outMembers);

			const auto& classMemberNode = listNode->children[1];

			// ClassMember -> AnnotationList [0] OptVisibility [1] ClassMemberDecl [2]
			const auto& optVisNode = classMemberNode->children[1];
			const auto& classMemberDeclNode = classMemberNode->children[2];
			const auto& actualMemberDecl = classMemberDeclNode->children[0];

			if (auto decl = ConvertActualDecl(actualMemberDecl.get()))
			{
				decl->visibility = ParseVisibility(optVisNode.get());
				outMembers.push_back(std::move(decl));
			}
		}
	}

	static void ExtractPrimaryCtorParams(const CstNode* listNode, ast::ClassDecl* classDecl, ast::ConstructorDecl* ctorDecl)
	{
		if (!listNode || listNode->symbol.Hashed() == "<EPSILON>"_hs || listNode->children.empty())
		{
			return;
		}

		if (listNode->children.size() == 3)
		{
			ExtractPrimaryCtorParams(listNode->children[0].get(), classDecl, ctorDecl);
			ProcessPrimaryCtorPar(listNode->children[2].get(), classDecl, ctorDecl);
		}
		else
		{
			ProcessPrimaryCtorPar(listNode->children[0].get(), classDecl, ctorDecl);
		}
	}

	static void ProcessPrimaryCtorPar(const CstNode* parNode, ast::ClassDecl* classDecl, ast::ConstructorDecl* ctorDecl)
	{
		const auto& optValVarNode = parNode->children[0];
		const re::String name = parNode->children[1]->token->lexeme;

		const auto typeNode = ConvertType(parNode->children[3].get());

		ast::FunDecl::Parameter param;
		param.name = name;
		param.type = typeNode->Clone();
		ctorDecl->parameters.push_back(std::move(param));

		if (!optValVarNode->children.empty() && optValVarNode->children[0]->symbol.Hashed() != "<EPSILON>"_hs)
		{ // Adding class field if parameter is val/var
			if (const re::String valVar = optValVarNode->children[0]->token->lexeme; valVar.Hashed() == "val"_hs)
			{ // val
				auto valDecl = std::make_unique<ast::ValDecl>();
				valDecl->name = name;
				valDecl->type = typeNode->Clone();
				valDecl->visibility = ast::Visibility::Public;
				classDecl->members.push_back(std::move(valDecl));
			}
			else if (valVar.Hashed() == "var"_hs)
			{ // var
				auto varDecl = std::make_unique<ast::VarDecl>();
				varDecl->name = name;
				varDecl->type = typeNode->Clone();
				varDecl->visibility = ast::Visibility::Public;
				classDecl->members.push_back(std::move(varDecl));
			}

			auto assignExpr = std::make_unique<ast::AssignExpr>();

			auto memAccess = std::make_unique<ast::MemberAccessExpr>();
			auto thisId = std::make_unique<ast::IdentifierExpr>();
			thisId->name = "this";
			memAccess->object = std::move(thisId);
			memAccess->member = name;
			assignExpr->target = std::move(memAccess);

			auto valId = std::make_unique<ast::IdentifierExpr>();
			valId->name = name;
			assignExpr->value = std::move(valId);

			auto exprStmt = std::make_unique<ast::ExprStmt>();
			exprStmt->expr = std::move(assignExpr);

			ctorDecl->body->statements.push_back(std::move(exprStmt));
		}
	}
};

} // namespace igni