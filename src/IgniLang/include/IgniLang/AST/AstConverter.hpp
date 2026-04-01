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

		if (!optPkg->children.empty() && optPkg->children[0]->symbol.Hashed() != "e"_hs)
		{
			// PackageDecl -> package PackagePath ;
			program->packageName = ExtractPathString(optPkg->children[0]->children[1].get());
		}

		ExtractImports(importsList.get(), program->imports);

		FlattenTopLevelDecls(decls.get(), program->statements);

		return program;
	}

private:
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
		if (!listNode || listNode->symbol.Hashed() == "e"_hs || listNode->children.empty())
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

	void FlattenTopLevelDecls(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol.Hashed() == "e"_hs || listNode->children.empty())
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

	std::unique_ptr<ast::Decl> ConvertTopLevelDecl(const CstNode* topLevelDecl)
	{
		const auto& declNode = topLevelDecl->children[2]; // TopLevelDecl -> AnnotationList OptVisibility Decl
		const auto& actualDecl = declNode->children[0]; // Decl -> ValDecl | FunDecl

		return ConvertActualDecl(actualDecl.get());
	}

	std::unique_ptr<ast::Decl> ConvertActualDecl(const CstNode* actualDecl)
	{
		switch (actualDecl->symbol.Hashed())
		{
		case "ValDecl"_hs:
		case "VarDecl"_hs: {
			auto decl = actualDecl->symbol.Hashed() == "ValDecl"_hs
				? std::unique_ptr<ast::Decl>(std::make_unique<ast::ValDecl>())
				: std::unique_ptr<ast::Decl>(std::make_unique<ast::VarDecl>());

			const auto& optColon = actualDecl->children[2];

			if (const auto val = dynamic_cast<ast::ValDecl*>(decl.get()))
			{
				val->name = actualDecl->children[1]->token->lexeme;
				val->initializer = ConvertExpr(actualDecl->children[4].get());
				if (optColon->children.size() == 2)
				{
					val->type = ConvertType(optColon->children[1].get());
				}
			}
			else if (const auto var = dynamic_cast<ast::VarDecl*>(decl.get()))
			{
				var->name = actualDecl->children[1]->token->lexeme;
				var->initializer = ConvertExpr(actualDecl->children[4].get());
				if (optColon->children.size() == 2)
				{
					var->type = ConvertType(optColon->children[1].get());
				}
			}
			return decl;
		}
		case "FunDecl"_hs: {
			auto funDecl = std::make_unique<ast::FunDecl>();

			if (const auto& optFunMod = actualDecl->children[0];
				!optFunMod->children.empty() && optFunMod->children[0]->symbol.Hashed() == "external"_hs)
			{ // External modifier
				funDecl->isExternal = true;
			}

			if (const auto& funNameNode = actualDecl->children[2]; funNameNode->children.size() == 1)
			{ // Name [2]
				funDecl->name = funNameNode->children[0]->token->lexeme;
			}
			else
			{
				funDecl->name = funNameNode->children.back()->token->lexeme;
			}

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
		default:
			return nullptr;
		}
	}

	std::unique_ptr<ast::Block> ConvertBlock(const CstNode* blockNode)
	{
		auto block = std::make_unique<ast::Block>();
		FlattenStatements(blockNode->children[1].get(), block->statements);
		return block;
	}

	void FlattenStatements(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol.Hashed() == "e"_hs || listNode->children.empty())
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
				if (stmtNode->children[1]->symbol.Hashed() != "e"_hs)
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
			// Rule 1: Designator -> ident
			if (exprNode->children.size() == 1)
			{
				auto id = std::make_unique<ast::IdentifierExpr>();
				id->name = exprNode->children[0]->token->lexeme;
				return id;
			}
			// Rule 2: Designator -> Designator(OptActPars)
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol.Hashed() == "("_hs)
			{
				auto call = std::make_unique<ast::CallExpr>();
				call->callee = ConvertExpr(exprNode->children[0].get());
				ExtractArguments(exprNode->children[2].get(), call->arguments);
				return call;
			}
			// Rule 3: Designator -> Designator[Expr]
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol.Hashed() == "["_hs)
			{
				auto idx = std::make_unique<ast::IndexExpr>();
				idx->array = ConvertExpr(exprNode->children[0].get()); // Что индексируем
				idx->index = ConvertExpr(exprNode->children[2].get()); // Сам индекс
				return idx;
			}
			if (exprNode->children.size() == 3 && exprNode->children[1]->symbol.Hashed() == "."_hs)
			{
				auto memberAccess = std::make_unique<ast::MemberAccessExpr>();
				memberAccess->object = ConvertExpr(exprNode->children[0].get()); // то, что слева от точки
				memberAccess->member = exprNode->children[2]->token->lexeme; // идентификатор справа
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
			if (exprNode->children[0]->symbol.Hashed() == "e"_hs)
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

	std::unique_ptr<ast::IfStmt> ConvertIfStmt(const CstNode* ifNode)
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

	std::unique_ptr<ast::WhileStmt> ConvertWhileStmt(const CstNode* whileNode)
	{
		auto stmt = std::make_unique<ast::WhileStmt>();
		stmt->condition = ConvertExpr(whileNode->children[2].get());
		stmt->body = ConvertBlock(whileNode->children[4].get());

		return stmt;
	}

	std::unique_ptr<ast::ForStmt> ConvertForStmt(const CstNode* forNode)
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

			if (const auto& optTypeList = typeNode->children[1]; optTypeList->children.size() == 1 && optTypeList->children[0]->symbol.Hashed() != "e"_hs)
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
		if (optFormPars->children.size() == 1 && optFormPars->children[0]->symbol.Hashed() == "e"_hs)
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
		if (optActPars->children.size() == 1 && optActPars->children[0]->symbol.Hashed() == "e"_hs)
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
};

} // namespace igni