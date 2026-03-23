#pragma once

#include "AstNodes.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace igni::compiler
{

class TextCompiler
{
public:
	std::string Compile(const ast::Program* program)
	{
		m_out.str("");
		m_out.clear();
		m_flatFunctions.clear();
		m_functionUpvalues.clear();
		m_currentFunction = nullptr;
		m_functionBoxedVars.clear();
		m_isTopLevel = false;
		m_labelCount = 0;

		// ==========================================
		// PASS 1: Scope & Capture Analysis
		// ==========================================
		std::unordered_set<std::string> globalScope;

		// Регистрируем все глобальные функции, чтобы они не считались Upvalues
		for (const auto& stmt : program->statements)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				globalScope.insert(fun->name.ToString());
			}
		}
		globalScope.insert("print");
		globalScope.insert("make_array");
		globalScope.insert("len");

		std::unordered_set<std::string> dummyFreeVars;
		for (const auto& stmt : program->statements)
		{
			ScanStatement(stmt.get(), globalScope, dummyFreeVars, globalScope, nullptr);
		}

		// ==========================================
		// PASS 2: Code Generation
		// ==========================================
		m_out << "// ==========================================\n";
		m_out << "// Auto-generated RVM Assembly\n";
		m_out << "// Package: " << program->packageName.ToString() << "\n";
		m_out << "// ==========================================\n\n";

		m_out << "// --- Entry Point ---\n";
		m_out << "CALL main 0\n";
		m_out << "RETURN\n\n";

		for (const ast::FunDecl* fun : m_flatFunctions)
		{
			CompileFunction(fun);
		}

		return m_out.str();
	}

private:
	int m_labelCount = 0;
	std::stringstream m_out;
	bool m_isTopLevel = true;

	std::vector<std::string> m_currentLocals;
	std::vector<std::string> m_currentUpvalues;

	std::vector<const ast::FunDecl*> m_flatFunctions;
	std::unordered_map<const ast::FunDecl*, std::vector<std::string>> m_functionUpvalues;

	std::unordered_map<const ast::FunDecl*, std::unordered_set<std::string>> m_functionBoxedVars;
	const ast::FunDecl* m_currentFunction = nullptr;

	// ==========================================
	// БЛОК 1: СЕМАНТИЧЕСКИЙ АНАЛИЗАТОР ЗАМЫКАНИЙ
	// ==========================================
	void ScanExpr(const ast::Expr* expr, const std::unordered_set<std::string>& localScope, std::unordered_set<std::string>& freeVars)
	{
		if (!expr)
			return;

		if (auto id = dynamic_cast<const ast::IdentifierExpr*>(expr))
		{
			if (!localScope.count(id->name.ToString()))
			{
				freeVars.insert(id->name.ToString());
			}
		}
		else if (auto bin = dynamic_cast<const ast::BinaryExpr*>(expr))
		{
			ScanExpr(bin->left.get(), localScope, freeVars);
			ScanExpr(bin->right.get(), localScope, freeVars);
		}
		else if (auto call = dynamic_cast<const ast::CallExpr*>(expr))
		{
			ScanExpr(call->callee.get(), localScope, freeVars);
			for (const auto& arg : call->arguments)
				ScanExpr(arg.get(), localScope, freeVars);
		}
		else if (auto asgn = dynamic_cast<const ast::AssignExpr*>(expr))
		{
			ScanExpr(asgn->target.get(), localScope, freeVars);
			ScanExpr(asgn->value.get(), localScope, freeVars);
		}
		// else if (auto idx = dynamic_cast<const ast::IndexExpr*>(expr))
		// {
		// 	ScanExpr(idx->array.get(), localScope, freeVars);
		// 	ScanExpr(idx->index.get(), localScope, freeVars);
		// }
	}

	void ScanStatement(const ast::Statement* stmt, std::unordered_set<std::string>& localScope, std::unordered_set<std::string>& freeVars, const std::unordered_set<std::string>& globalScope, const ast::FunDecl* currentFunContext)
	{
		if (!stmt)
			return;

		if (auto v = dynamic_cast<const ast::ValDecl*>(stmt))
		{
			ScanExpr(v->initializer.get(), localScope, freeVars);
			localScope.insert(v->name.ToString());
		}
		else if (auto v = dynamic_cast<const ast::VarDecl*>(stmt))
		{
			ScanExpr(v->initializer.get(), localScope, freeVars);
			localScope.insert(v->name.ToString());
		}
		else if (auto e = dynamic_cast<const ast::ExprStmt*>(stmt))
		{
			ScanExpr(e->expr.get(), localScope, freeVars);
		}
		else if (auto r = dynamic_cast<const ast::ReturnStmt*>(stmt))
		{
			ScanExpr(r->expr.get(), localScope, freeVars);
		}
		else if (auto block = dynamic_cast<const ast::Block*>(stmt))
		{
			std::unordered_set<std::string> blockScope = localScope;
			for (const auto& s : block->statements)
			{
				ScanStatement(s.get(), blockScope, freeVars, globalScope, currentFunContext);
			}
		}
		else if (auto fun = dynamic_cast<const ast::FunDecl*>(stmt))
		{
			// ИСПРАВЛЕНИЕ 1: Добавляем саму функцию в родительский Scope
			localScope.insert(fun->name.ToString());

			// ИСПРАВЛЕНИЕ 2: Вложенная функция НЕ наследует родительские переменные как локальные!
			// У неё абсолютно чистый scope, только параметры и её собственное имя (для рекурсии).
			std::unordered_set<std::string> funLocals;
			funLocals.insert(fun->name.ToString());

			for (const auto& p : fun->parameters)
				funLocals.insert(p.name.ToString());

			std::unordered_set<std::string> funFreeVars;
			if (fun->body)
			{
				ScanStatement(fun->body.get(), funLocals, funFreeVars, globalScope, fun);
			}

			std::vector<std::string> upvals;
			for (const auto& uv : funFreeVars)
			{
				if (!globalScope.count(uv))
				{
					upvals.push_back(uv); // Сохраняем порядок Upvalues

					// Просим РОДИТЕЛЯ запаковать эту переменную в BOX
					if (currentFunContext)
					{
						m_functionBoxedVars[currentFunContext].insert(uv);
					}

					// Если родитель сам не владеет этой переменной, пробрасываем выше
					if (!localScope.count(uv))
						freeVars.insert(uv);
				}
			}

			m_functionUpvalues[fun] = upvals;
			m_flatFunctions.push_back(fun);
		}
		else if (auto ifs = dynamic_cast<const ast::IfStmt*>(stmt))
		{
			ScanExpr(ifs->condition.get(), localScope, freeVars);
			if (ifs->thenBranch)
				ScanStatement(ifs->thenBranch.get(), localScope, freeVars, globalScope, currentFunContext);
			if (ifs->elseBranch)
				ScanStatement(ifs->elseBranch.get(), localScope, freeVars, globalScope, currentFunContext);
		}
		else if (auto whl = dynamic_cast<const ast::WhileStmt*>(stmt))
		{
			ScanExpr(whl->condition.get(), localScope, freeVars);
			if (whl->body)
				ScanStatement(whl->body.get(), localScope, freeVars, globalScope, currentFunContext);
		}
		else if (auto fr = dynamic_cast<const ast::ForStmt*>(stmt))
		{
			ScanExpr(fr->startExpr.get(), localScope, freeVars);
			ScanExpr(fr->endExpr.get(), localScope, freeVars);

			std::unordered_set<std::string> forScope = localScope;
			forScope.insert(fr->iteratorName.ToString()); // Итератор виден только внутри For

			if (fr->body)
				ScanStatement(fr->body.get(), forScope, freeVars, globalScope, currentFunContext);
		}
	}

	// ==========================================
	// БЛОК 2: ГЕНЕРАТОР КОДА
	// ==========================================
	void CompileFunction(const ast::FunDecl* fun)
	{
		m_currentFunction = fun;
		m_out << "FUN " << fun->name.ToString() << "\n";
		m_currentLocals.clear();
		m_currentUpvalues = m_functionUpvalues[fun];

		const auto& boxedVars = m_functionBoxedVars[fun];

		for (auto it = fun->parameters.rbegin(); it != fun->parameters.rend(); ++it)
		{
			std::string paramName = it->name.ToString();
			if (boxedVars.count(paramName))
			{
				m_out << "BOX\n";
			}
			m_out << "SET " << paramName << "\n";
			m_currentLocals.push_back(paramName);
		}

		m_isTopLevel = false;
		if (fun->body)
		{
			for (const auto& s : fun->body->statements)
				CompileStatement(s.get());
		}
		m_isTopLevel = true;

		m_out << "CONST 0\nRETURN\n\n";
	}

	void CompileStatement(const ast::Statement* stmt)
	{
		if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt))
		{
			if (varDecl->initializer)
				CompileExpr(varDecl->initializer.get());
			else
				m_out << "CONST 0\n";

			if (m_functionBoxedVars[m_currentFunction].count(varDecl->name.ToString()))
			{
				m_out << "BOX\n";
			}

			m_out << "SET " << varDecl->name.ToString() << "\n";
			m_currentLocals.push_back(varDecl->name.ToString());
		}
		else if (const auto exprStmt = dynamic_cast<const ast::ExprStmt*>(stmt))
		{
			CompileExpr(exprStmt->expr.get());

			bool shouldPop = true;
			if (auto call = dynamic_cast<const ast::CallExpr*>(exprStmt->expr.get()))
			{
				if (auto id = dynamic_cast<const ast::IdentifierExpr*>(call->callee.get()))
				{
					if (id->name.ToString() == "print")
						shouldPop = false;
				}
			}
			if (shouldPop)
				m_out << "POP\n";
		}
		else if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(stmt))
		{
			if (retStmt->expr)
				CompileExpr(retStmt->expr.get());
			else
				m_out << "CONST 0\n";
			m_out << "RETURN\n";
		}
		else if (const auto funDecl = dynamic_cast<const ast::FunDecl*>(stmt))
		{
			const auto& upvals = m_functionUpvalues[funDecl];

			// Собираем ячейки для замыкания
			for (const auto& uv : upvals)
			{
				auto upIt = std::find(m_currentUpvalues.begin(), m_currentUpvalues.end(), uv);
				if (upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					m_out << "GET " << uv << "\n";
				}
			}

			m_out << "MAKE_CLOSURE " << funDecl->name.ToString() << " " << upvals.size() << "\n";
			m_out << "SET " << funDecl->name.ToString() << "\n";
			m_currentLocals.push_back(funDecl->name.ToString());
		}
		else if (const auto blockStmt = dynamic_cast<const ast::Block*>(stmt))
		{
			for (const auto& s : blockStmt->statements)
				CompileStatement(s.get());
		}
		else if (const auto whileStmt = dynamic_cast<const ast::WhileStmt*>(stmt))
		{
			int labelId = m_labelCount++;
			std::string startLabel = "L_while_start_" + std::to_string(labelId);
			std::string endLabel = "L_while_end_" + std::to_string(labelId);

			m_out << "LABEL " << startLabel << "\n";
			CompileExpr(whileStmt->condition.get());
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			if (whileStmt->body)
			{
				for (const auto& s : whileStmt->body->statements)
					CompileStatement(s.get());
			}

			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
		else if (const auto forStmt = dynamic_cast<const ast::ForStmt*>(stmt))
		{
			int labelId = m_labelCount++;
			std::string startLabel = "L_for_start_" + std::to_string(labelId);
			std::string endLabel = "L_for_end_" + std::to_string(labelId);

			std::string iterName = forStmt->iteratorName.ToString();
			std::string limitName = "_for_limit_" + std::to_string(labelId);

			m_out << "// --- for (" << iterName << ") ---\n";
			CompileExpr(forStmt->startExpr.get());
			m_out << "SET " << iterName << "\n";
			m_currentLocals.push_back(iterName);

			CompileExpr(forStmt->endExpr.get());
			m_out << "SET " << limitName << "\n";
			m_currentLocals.push_back(limitName);

			m_out << "LABEL " << startLabel << "\n";
			m_out << "CONST 1\n";
			m_out << "GET " << limitName << "\n";
			m_out << "GET " << iterName << "\n";
			m_out << "LESS\n";
			m_out << "SUB\n";
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			if (forStmt->body)
			{
				for (const auto& s : forStmt->body->statements)
					CompileStatement(s.get());
			}

			m_out << "GET " << iterName << "\n";
			m_out << "CONST 1\n";
			m_out << "ADD\n";
			m_out << "SET " << iterName << "\n";
			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
	}

	void CompileExpr(const ast::Expr* expr)
	{
		if (!expr)
			return;

		if (const auto litExpr = dynamic_cast<const ast::LiteralExpr*>(expr))
		{
			m_out << "CONST " << litExpr->token.lexeme << "\n";
			return;
		}

		if (const auto idExpr = dynamic_cast<const ast::IdentifierExpr*>(expr))
		{
			std::string name = idExpr->name.ToString();

			auto upIt = std::find(m_currentUpvalues.begin(), m_currentUpvalues.end(), name);
			if (upIt != m_currentUpvalues.end())
			{
				m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else if (std::find(m_currentLocals.begin(), m_currentLocals.end(), name) != m_currentLocals.end())
			{
				m_out << "GET " << name << "\n";
				if (m_functionBoxedVars[m_currentFunction].count(name))
					m_out << "UNBOX\n";
			}
			else
			{
				m_out << "LOAD_FUN " << name << "\n";
			}
			return;
		}

		if (const auto assignExpr = dynamic_cast<const ast::AssignExpr*>(expr))
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(assignExpr->target.get()))
			{
				std::string name = id->name.ToString();
				auto upIt = std::find(m_currentUpvalues.begin(), m_currentUpvalues.end(), name);

				if (upIt != m_currentUpvalues.end())
				{
					CompileExpr(assignExpr->value.get());
					m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					if (m_functionBoxedVars[m_currentFunction].count(name))
					{
						m_out << "GET " << name << "\n";
						CompileExpr(assignExpr->value.get());
						m_out << "STORE_BOX\n";
					}
					else
					{
						CompileExpr(assignExpr->value.get());
						m_out << "SET " << name << "\n";
					}
				}
				m_out << "CONST 0\n";
			}
			return;
		}

		if (const auto callExpr = dynamic_cast<const ast::CallExpr*>(expr))
		{
			bool isDirectCall = false;
			std::string directFuncName = "";
			if (auto id = dynamic_cast<const ast::IdentifierExpr*>(callExpr->callee.get()))
			{
				directFuncName = id->name.ToString();
				if (directFuncName == "print" || directFuncName == "make_array" || directFuncName == "len")
					isDirectCall = true;
				else if (std::find(m_currentLocals.begin(), m_currentLocals.end(), directFuncName) == m_currentLocals.end() && std::find(m_currentUpvalues.begin(), m_currentUpvalues.end(), directFuncName) == m_currentUpvalues.end())
				{
					isDirectCall = true;
				}
			}

			if (isDirectCall)
			{
				for (const auto& arg : callExpr->arguments)
					CompileExpr(arg.get());
				if (directFuncName == "print" || directFuncName == "len")
					m_out << "NATIVE \"" << directFuncName << "\" " << callExpr->arguments.size() << "\n";
				else
					m_out << "CALL " << directFuncName << " " << callExpr->arguments.size() << "\n";
			}
			else
			{
				CompileExpr(callExpr->callee.get());
				for (const auto& arg : callExpr->arguments)
					CompileExpr(arg.get());
				m_out << "CALL_INDIRECT " << callExpr->arguments.size() << "\n";
			}
			return;
		}

		if (const auto binExpr = dynamic_cast<const ast::BinaryExpr*>(expr))
		{
			CompileExpr(binExpr->left.get());
			CompileExpr(binExpr->right.get());

			const std::string op = binExpr->op.ToString();

			// clang-format off
          	if (op == "+")       m_out << "ADD\n";
          	else if (op == "-")  m_out << "SUB\n";
          	else if (op == "*")  m_out << "MUL\n";
          	else if (op == "/")  m_out << "DIV\n";
          	else if (op == "<")  m_out << "LESS\n";
          	else if (op == "==") m_out << "EQUAL\n";
          	else if (op == "&&") m_out << "AND\n";
          	else if (op == "||") m_out << "OR\n";
          	else
          	{
          	   throw std::runtime_error("TextCompiler: Unsupported binary operator '" + op + "'");
          	}
			// clang-format on

			return;
		}

		std::cerr << "Warning: Unhandled expression type in TextCompiler.\n";
	}
};

} // namespace igni::compiler