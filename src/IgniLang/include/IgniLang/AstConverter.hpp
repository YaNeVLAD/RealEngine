#pragma once

#include "AstNodes.hpp"
#include <IgniLang/CstNode.hpp>
#include <iostream>
#include <stdexcept>

namespace igni
{

class AstConverter
{
public:
	std::unique_ptr<ast::Program> Convert(const CstNode* root)
	{
		if (!root || root->symbol != "Program")
			return nullptr;

		auto program = std::make_unique<ast::Program>();

		const auto& optPkg = root->children[0];
		const auto& decls = root->children[2]; // TopLevelDeclList

		if (optPkg->children.size() > 0 && optPkg->children[0]->symbol != "e")
		{
			program->packageName = optPkg->children[0]->children[1]->children[0]->token->lexeme;
		}

		FlattenTopLevelDecls(decls.get(), program->statements);
		return program;
	}

private:
	void FlattenTopLevelDecls(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol == "e" || listNode->children.empty())
			return;

		if (listNode->children.size() == 2)
		{
			FlattenTopLevelDecls(listNode->children[0].get(), outStmts);

			// Вызываем правильный метод для TopLevelDecl
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
		// --- VAL / VAR ---
		if (actualDecl->symbol == "ValDecl" || actualDecl->symbol == "VarDecl")
		{
			auto decl = actualDecl->symbol == "ValDecl"
				? std::unique_ptr<ast::Decl>(std::make_unique<ast::ValDecl>())
				: std::unique_ptr<ast::Decl>(std::make_unique<ast::VarDecl>());

			// ValDecl -> val ident OptColonType = Expr ;
			// Индексы:   [0]  [1]       [2]    [3]  [4] [5]
			const auto& optColon = actualDecl->children[2];
			if (const auto val = dynamic_cast<ast::ValDecl*>(decl.get()))
			{
				val->name = actualDecl->children[1]->token->lexeme;
				val->initializer = ConvertExpr(actualDecl->children[4].get());
				if (optColon->children.size() == 2) // : Type
				{
					val->type = ConvertType(optColon->children[1].get());
				}
			}
			else if (const auto var = dynamic_cast<ast::VarDecl*>(decl.get()))
			{
				var->name = actualDecl->children[1]->token->lexeme;
				var->initializer = ConvertExpr(actualDecl->children[4].get());
				if (optColon->children.size() == 2) // : Type
				{
					val->type = ConvertType(optColon->children[1].get());
				}
			}
			return decl;
		}

		// --- FUN ---
		if (actualDecl->symbol == "FunDecl")
		{
			auto funDecl = std::make_unique<ast::FunDecl>();

			// 1. Имя функции [2]
			const auto& funNameNode = actualDecl->children[2];
			if (funNameNode->children.size() == 1)
			{
				funDecl->name = funNameNode->children[0]->token->lexeme;
			}
			else
			{
				funDecl->name = funNameNode->children.back()->token->lexeme;
			}

			const auto& optColon = actualDecl->children[6];
			if (optColon->children.size() == 2)
			{
				funDecl->returnType = ConvertType(optColon->children[1].get());
			}

			// 2. Параметры [5]
			ExtractParameters(actualDecl->children[5].get(), funDecl->parameters);

			// 3. Тело функции (ИСПРАВЛЕНО: ТЕПЕРЬ ИНДЕКС 8)
			const auto& funBody = actualDecl->children[8];
			if (funBody->children[0]->symbol == "Block")
			{
				funDecl->body = ConvertBlock(funBody->children[0].get());
			}
			return funDecl;
		}

		return nullptr;
	}

	std::unique_ptr<ast::Decl> ConvertDecl(const CstNode* topLevelDecl)
	{
		const auto& declNode = topLevelDecl->children[2];
		const auto& actualDecl = declNode->children[0];

		// --- VAL / VAR ---
		if (actualDecl->symbol == "ValDecl" || actualDecl->symbol == "VarDecl")
		{
			auto decl = actualDecl->symbol == "ValDecl"
				? std::unique_ptr<ast::Decl>(std::make_unique<ast::ValDecl>())
				: std::unique_ptr<ast::Decl>(std::make_unique<ast::VarDecl>());

			if (const auto val = dynamic_cast<ast::ValDecl*>(decl.get()))
			{
				val->name = actualDecl->children[1]->token->lexeme;
				val->initializer = ConvertExpr(actualDecl->children[4].get());
			}
			else if (const auto var = dynamic_cast<ast::VarDecl*>(decl.get()))
			{
				var->name = actualDecl->children[1]->token->lexeme;
				var->initializer = ConvertExpr(actualDecl->children[4].get());
			}
			return decl;
		}

		// --- FUN ---
		if (actualDecl->symbol == "FunDecl")
		{
			auto funDecl = std::make_unique<ast::FunDecl>();

			// FunDecl -> OptFunMod fun FunName OptTypeParams ( OptFormPars ) OptColonType FunBody
			// Индексы:    [0]     [1]    [2]       [3]      [4]      [5]       [6]        [7]

			// 1. Извлекаем имя функции из узла FunName [2]
			const auto& funNameNode = actualDecl->children[2];
			if (funNameNode->children.size() == 1)
			{
				// FunName -> ident
				funDecl->name = funNameNode->children[0]->token->lexeme;
			}
			else
			{
				// FunName -> Type . ident | Type ? . ident
				// Имя всегда лежит в самом последнем ребенке
				funDecl->name = funNameNode->children.back()->token->lexeme;
			}

			// 2. Извлекаем параметры из OptFormPars [5]
			ExtractParameters(actualDecl->children[5].get(), funDecl->parameters);

			// 3. Извлекаем тело из FunBody [7]
			const auto& funBody = actualDecl->children[7];
			if (funBody->children[0]->symbol == "Block")
			{
				funDecl->body = ConvertBlock(funBody->children[0].get());
			}
			return funDecl;
		}

		return nullptr;
	}

	std::unique_ptr<ast::Block> ConvertBlock(const CstNode* blockNode)
	{
		auto block = std::make_unique<ast::Block>();
		FlattenStatements(blockNode->children[1].get(), block->statements);
		return block;
	}

	void FlattenStatements(const CstNode* listNode, std::vector<std::unique_ptr<ast::Statement>>& outStmts)
	{
		if (listNode->symbol == "e" || listNode->children.empty())
			return;

		if (listNode->children.size() == 2)
		{
			FlattenStatements(listNode->children[0].get(), outStmts);

			const auto& stmtNode = listNode->children[1];
			const auto& actualStmt = stmtNode->children[0];

			if (actualStmt->symbol == "ValDecl" || actualStmt->symbol == "VarDecl" || actualStmt->symbol == "FunDecl")
			{
				// Statement -> VarDecl (у него нет обертки TopLevelDecl, парсим напрямую)
				outStmts.push_back(ConvertActualDecl(actualStmt.get()));
			}
			else if (actualStmt->symbol == "return")
			{
				auto ret = std::make_unique<ast::ReturnStmt>();
				if (stmtNode->children[1]->symbol != "e")
				{
					ret->expr = ConvertExpr(stmtNode->children[1]->children[0].get());
				}
				outStmts.push_back(std::move(ret));
			}
			else if (actualStmt->symbol == "Expr")
			{
				auto exprStmt = std::make_unique<ast::ExprStmt>();
				exprStmt->expr = ConvertExpr(actualStmt.get());
				outStmts.push_back(std::move(exprStmt));
			}
			else if (actualStmt->symbol == "IfStmt")
			{
				outStmts.push_back(ConvertIfStmt(actualStmt.get()));
			}
			else if (actualStmt->symbol == "WhileStmt")
			{
				outStmts.push_back(ConvertWhileStmt(actualStmt.get()));
			}
			else if (actualStmt->symbol == "ForStmt")
			{
				outStmts.push_back(ConvertForStmt(actualStmt.get()));
			}
		}
	}

	// Универсальный конвертер для каскада выражений
	static std::unique_ptr<ast::Expr> ConvertExpr(const CstNode* exprNode)
	{
		if (!exprNode)
			return nullptr;

		// 1. СНАЧАЛА проверяем конкретные типы узлов
		if (exprNode->symbol == "Designator")
		{
			// Правило: Designator -> ident
			if (exprNode->children.size() == 1)
			{
				auto id = std::make_unique<ast::IdentifierExpr>();
				id->name = exprNode->children[0]->token->lexeme;
				return id;
			}
			// Правило: Designator -> Designator ( OptActPars )
			if (exprNode->children.size() == 4 && exprNode->children[1]->symbol == "(")
			{
				auto call = std::make_unique<ast::CallExpr>();
				call->callee = ConvertExpr(exprNode->children[0].get()); // Что вызываем

				// Извлекаем аргументы
				ExtractArguments(exprNode->children[2].get(), call->arguments);
				return call;
			}
		}

		// --- 2. Обработка PrimaryExpr ---
		if (exprNode->symbol == "PrimaryExpr")
		{
			const auto& child = exprNode->children[0];

			if (child->symbol == "true" || child->symbol == "false" || child->symbol == "intCon" || child->symbol == "floatCon" || child->symbol == "stringCon")
			{
				auto lit = std::make_unique<ast::LiteralExpr>();
				lit->token = *child->token;
				return lit;
			}
			if (child->symbol == "(")
			{
				return ConvertExpr(exprNode->children[1].get());
			}
			if (child->symbol == "Designator")
			{
				// ПЕРЕДАЕМ УПРАВЛЕНИЕ БЛОКУ ВЫШЕ!
				return ConvertExpr(child.get());
			}
		}

		// 2. ЗАТЕМ делаем "проброс" для транзитных узлов (AddExpr -> MulExpr и т.д.)
		if (exprNode->children.size() == 1)
		{
			if (exprNode->children[0]->symbol == "e")
			{
				return nullptr;
			}
			return ConvertExpr(exprNode->children[0].get());
		}

		// Правило: Expr -> LogOrExpr = Expr
		if (exprNode->children.size() == 3 && exprNode->children[1]->symbol == "=")
		{
			auto assign = std::make_unique<ast::AssignExpr>();
			assign->target = ConvertExpr(exprNode->children[0].get());
			assign->value = ConvertExpr(exprNode->children[2].get());
			return assign;
		}

		// 3. Бинарные операции
		if (exprNode->children.size() == 3)
		{
			auto binExpr = std::make_unique<ast::BinaryExpr>();
			binExpr->left = ConvertExpr(exprNode->children[0].get());
			binExpr->op = exprNode->children[1]->symbol; // Оператор всегда посередине
			binExpr->right = ConvertExpr(exprNode->children[2].get());
			return binExpr;
		}

		std::cerr << "Unhandled Expr node: " << exprNode->symbol << "\n";
		return nullptr;
	}

	std::unique_ptr<ast::IfStmt> ConvertIfStmt(const CstNode* ifNode)
	{
		auto ifStmt = std::make_unique<ast::IfStmt>();

		// if ( Expr ) Block OptElse
		// [0] [1] [2] [3]  [4]    [5]
		ifStmt->condition = ConvertExpr(ifNode->children[2].get());

		const auto& blockNode = ifNode->children[4];
		ifStmt->thenBranch = ConvertBlock(blockNode.get());

		// Разбираем OptElse
		const auto& optElse = ifNode->children[5];
		if (optElse->children.size() == 2) // else ElseBody
		{
			const auto& elseBody = optElse->children[1]->children[0];
			if (elseBody->symbol == "IfStmt")
			{
				ifStmt->elseBranch = ConvertIfStmt(elseBody.get()); // else if
			}
			else if (elseBody->symbol == "Block")
			{
				ifStmt->elseBranch = ConvertBlock(elseBody.get()); // else { ... }
			}
		}

		return ifStmt;
	}

	std::unique_ptr<ast::WhileStmt> ConvertWhileStmt(const CstNode* whileNode)
	{
		auto stmt = std::make_unique<ast::WhileStmt>();
		// while ( Expr ) Block
		// [0]   [1] [2] [3] [4]
		stmt->condition = ConvertExpr(whileNode->children[2].get());
		stmt->body = ConvertBlock(whileNode->children[4].get());
		return stmt;
	}

	std::unique_ptr<ast::ForStmt> ConvertForStmt(const CstNode* forNode)
	{
		auto stmt = std::make_unique<ast::ForStmt>();
		// for ( ident in Expr .. Expr ) Block
		// [0] [1] [2]  [3] [4] [5] [6] [7] [8]
		stmt->iteratorName = forNode->children[2]->token->lexeme;
		stmt->startExpr = ConvertExpr(forNode->children[4].get());
		stmt->endExpr = ConvertExpr(forNode->children[6].get());
		stmt->body = ConvertBlock(forNode->children[8].get());
		return stmt;
	}

	static std::unique_ptr<ast::TypeNode> ConvertType(const CstNode* typeNode)
	{
		if (!typeNode)
			return nullptr;

		// Type -> ident OptTypeArgs OptQuestion
		if (typeNode->children[0]->symbol == "ident")
		{
			auto simpleType = std::make_unique<ast::SimpleTypeNode>();
			simpleType->name = typeNode->children[0]->token->lexeme;

			// Парсим OptTypeArgs [1]
			const auto& optTypeArgs = typeNode->children[1];
			if (optTypeArgs->children.size() == 3) // < TypeList >
			{
				ExtractTypeList(optTypeArgs->children[1].get(), simpleType->typeArgs);
			}

			// Парсим OptQuestion [2]
			simpleType->isNullable = (typeNode->children[2]->children.size() > 0 && typeNode->children[2]->children[0]->symbol == "?");

			return simpleType;
		}
		// Type -> ( OptTypeList ) -> Type OptQuestion
		//         [0]    [1]     [2][3] [4]     [5]
		else if (typeNode->children[0]->symbol == "(")
		{
			auto funType = std::make_unique<ast::FunctionTypeNode>();

			const auto& optTypeList = typeNode->children[1];
			if (optTypeList->children.size() == 1 && optTypeList->children[0]->symbol != "e")
			{
				ExtractTypeList(optTypeList->children[0].get(), funType->paramTypes);
			}

			funType->returnType = ConvertType(typeNode->children[4].get());
			funType->isNullable = (!typeNode->children[5]->children.empty() && typeNode->children[5]->children[0]->symbol == "?");

			return funType;
		}

		return nullptr;
	}

	// Вспомогательный метод для списков типов (TypeList -> TypeList , Type | Type)
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

	static void ExtractParameters(const CstNode* optFormPars, std::vector<ast::Parameter>& outParams)
	{
		if (optFormPars->children.size() == 1 && optFormPars->children[0]->symbol == "e")
			return;
		FlattenFormPars(optFormPars->children[0].get(), outParams);
	}

	static void FlattenFormPars(const CstNode* formPars, std::vector<ast::Parameter>& outParams)
	{
		if (formPars->children.size() == 3)
		{
			FlattenFormPars(formPars->children[0].get(), outParams);
			// FormPar -> ident : Type
			const auto& parNode = formPars->children[2];
			outParams.push_back({ parNode->children[0]->token->lexeme, ConvertType(parNode->children[2].get()) });
		}
		else if (formPars->children.size() == 1)
		{
			const auto& parNode = formPars->children[0];
			outParams.push_back({ parNode->children[0]->token->lexeme, ConvertType(parNode->children[2].get()) });
		}
	}

	static void ExtractArguments(const CstNode* optActPars, std::vector<std::unique_ptr<ast::Expr>>& outArgs)
	{
		if (optActPars->children.size() == 1 && optActPars->children[0]->symbol == "e")
			return;
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