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
	static void ScanExpr(const ast::Expr* expr, const std::unordered_set<std::string>& localScope, std::unordered_set<std::string>& freeVars)
	{
		if (!expr)
		{
			return;
		}

		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(expr))
		{
			if (!localScope.contains(id->name.ToString()))
			{
				freeVars.insert(id->name.ToString());
			}
		}
		else if (const auto bin = dynamic_cast<const ast::BinaryExpr*>(expr))
		{
			ScanExpr(bin->left.get(), localScope, freeVars);
			ScanExpr(bin->right.get(), localScope, freeVars);
		}
		else if (const auto call = dynamic_cast<const ast::CallExpr*>(expr))
		{
			ScanExpr(call->callee.get(), localScope, freeVars);
			for (const auto& arg : call->arguments)
			{
				ScanExpr(arg.get(), localScope, freeVars);
			}
		}
		else if (const auto assign = dynamic_cast<const ast::AssignExpr*>(expr))
		{
			ScanExpr(assign->target.get(), localScope, freeVars);
			ScanExpr(assign->value.get(), localScope, freeVars);
		}
		else if (const auto un = dynamic_cast<const ast::UnaryExpr*>(expr))
		{
			ScanExpr(un->operand.get(), localScope, freeVars);
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
		{
			return;
		}

		if (const auto val = dynamic_cast<const ast::ValDecl*>(stmt))
		{
			ScanExpr(val->initializer.get(), localScope, freeVars);
			localScope.insert(val->name.ToString());
		}
		else if (const auto var = dynamic_cast<const ast::VarDecl*>(stmt))
		{
			ScanExpr(var->initializer.get(), localScope, freeVars);
			localScope.insert(var->name.ToString());
		}
		else if (const auto e = dynamic_cast<const ast::ExprStmt*>(stmt))
		{
			ScanExpr(e->expr.get(), localScope, freeVars);
		}
		else if (const auto r = dynamic_cast<const ast::ReturnStmt*>(stmt))
		{
			ScanExpr(r->expr.get(), localScope, freeVars);
		}
		else if (const auto block = dynamic_cast<const ast::Block*>(stmt))
		{
			std::unordered_set<std::string> blockScope = localScope;
			for (const auto& s : block->statements)
			{
				ScanStatement(s.get(), blockScope, freeVars, globalScope, currentFunContext);
			}
		}
		else if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt))
		{
			localScope.insert(fun->name.ToString());

			std::unordered_set<std::string> funLocals;
			funLocals.insert(fun->name.ToString());

			for (const auto& p : fun->parameters)
			{
				funLocals.insert(p.name.ToString());
			}

			std::unordered_set<std::string> funFreeVars;
			if (fun->body)
			{
				ScanStatement(fun->body.get(), funLocals, funFreeVars, globalScope, fun);
			}

			std::vector<std::string> upvalues;
			for (const auto& uv : funFreeVars)
			{
				if (!globalScope.contains(uv))
				{
					upvalues.push_back(uv);

					if (currentFunContext)
					{
						m_functionBoxedVars[currentFunContext].insert(uv);
					}

					if (!localScope.contains(uv))
					{
						freeVars.insert(uv);
					}
				}
			}

			m_functionUpvalues[fun] = upvalues;
			m_flatFunctions.push_back(fun);
		}
		else if (const auto ifs = dynamic_cast<const ast::IfStmt*>(stmt))
		{
			ScanExpr(ifs->condition.get(), localScope, freeVars);
			if (ifs->thenBranch)
			{
				ScanStatement(ifs->thenBranch.get(), localScope, freeVars, globalScope, currentFunContext);
			}
			if (ifs->elseBranch)
			{
				ScanStatement(ifs->elseBranch.get(), localScope, freeVars, globalScope, currentFunContext);
			}
		}
		else if (const auto whl = dynamic_cast<const ast::WhileStmt*>(stmt))
		{
			ScanExpr(whl->condition.get(), localScope, freeVars);
			if (whl->body)
			{
				ScanStatement(whl->body.get(), localScope, freeVars, globalScope, currentFunContext);
			}
		}
		else if (const auto fr = dynamic_cast<const ast::ForStmt*>(stmt))
		{
			ScanExpr(fr->startExpr.get(), localScope, freeVars);
			ScanExpr(fr->endExpr.get(), localScope, freeVars);

			std::unordered_set<std::string> forScope = localScope;
			forScope.insert(fr->iteratorName.ToString());

			if (fr->body)
			{
				ScanStatement(fr->body.get(), forScope, freeVars, globalScope, currentFunContext);
			}
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
			if (boxedVars.contains(paramName))
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
			{
				CompileStatement(s.get());
			}
		}
		m_isTopLevel = true;

		m_out << "CONST 0\nRETURN\n\n";
	}

	void CompileStatement(const ast::Statement* stmt)
	{
		if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt))
		{
			if (varDecl->initializer)
			{
				CompileExpr(varDecl->initializer.get());
			}
			else
			{
				m_out << "CONST 0\n";
			}

			if (m_functionBoxedVars[m_currentFunction].contains(varDecl->name.ToString()))
			{
				m_out << "BOX\n";
			}

			m_out << "SET " << varDecl->name.ToString() << "\n";
			m_currentLocals.push_back(varDecl->name.ToString());
		}
		else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt))
		{
			if (valDecl->initializer)
			{
				CompileExpr(valDecl->initializer.get());
			}
			else
			{
				m_out << "CONST 0\n";
			}

			if (m_functionBoxedVars[m_currentFunction].contains(valDecl->name.ToString()))
			{
				m_out << "BOX\n";
			}

			m_out << "SET " << valDecl->name.ToString() << "\n";
			m_currentLocals.push_back(valDecl->name.ToString());
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
					{
						shouldPop = false;
					}
				}
			}
			if (shouldPop)
			{
				m_out << "POP\n";
			}
		}
		else if (const auto retStmt = dynamic_cast<const ast::ReturnStmt*>(stmt))
		{
			if (retStmt->expr)
			{
				CompileExpr(retStmt->expr.get());
			}
			else
			{
				m_out << "CONST 0\n";
			}
			m_out << "RETURN\n";
		}
		else if (const auto funDecl = dynamic_cast<const ast::FunDecl*>(stmt))
		{
			const auto& upvalues = m_functionUpvalues[funDecl];

			for (const auto& uv : upvalues)
			{
				if (auto upIt = std::ranges::find(m_currentUpvalues, uv); upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					m_out << "GET " << uv << "\n";
				}
			}

			m_out << "MAKE_CLOSURE " << funDecl->name.ToString() << " " << upvalues.size() << "\n";
			m_out << "SET " << funDecl->name.ToString() << "\n";
			m_currentLocals.push_back(funDecl->name.ToString());
		}
		else if (const auto blockStmt = dynamic_cast<const ast::Block*>(stmt))
		{
			for (const auto& s : blockStmt->statements)
			{
				CompileStatement(s.get());
			}
		}
		else if (const auto ifStmt = dynamic_cast<const ast::IfStmt*>(stmt))
		{
			auto currentLabelId = m_labelCount++;
			std::string elseLabel = "L_else_" + std::to_string(currentLabelId);
			std::string endLabel = "L_end_" + std::to_string(currentLabelId);

			m_out << "// --- if ---\n";
			CompileExpr(ifStmt->condition.get());

			if (ifStmt->elseBranch)
			{
				m_out << "JMP_IF_FALSE " << elseLabel << "\n";
			}
			else
			{
				m_out << "JMP_IF_FALSE " << endLabel << "\n";
			}

			if (ifStmt->thenBranch)
			{
				for (const auto& s : ifStmt->thenBranch->statements)
					CompileStatement(s.get());
			}

			if (ifStmt->elseBranch)
			{
				m_out << "JMP " << endLabel << "\n";
				m_out << "LABEL " << elseLabel << "\n";
				CompileStatement(ifStmt->elseBranch.get());
			}

			m_out << "LABEL " << endLabel << "\n";
		}
		else if (const auto whileStmt = dynamic_cast<const ast::WhileStmt*>(stmt))
		{
			auto labelId = m_labelCount++;
			auto startLabel = "L_while_start_" + std::to_string(labelId);
			auto endLabel = "L_while_end_" + std::to_string(labelId);

			m_out << "LABEL " << startLabel << "\n";
			CompileExpr(whileStmt->condition.get());
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			if (whileStmt->body)
			{
				for (const auto& s : whileStmt->body->statements)
				{
					CompileStatement(s.get());
				}
			}

			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
		else if (const auto forStmt = dynamic_cast<const ast::ForStmt*>(stmt))
		{
			auto labelId = m_labelCount++;
			auto startLabel = "L_for_start_" + std::to_string(labelId);
			auto endLabel = "L_for_end_" + std::to_string(labelId);

			auto iterName = forStmt->iteratorName.ToString();
			auto limitName = "_for_limit_" + std::to_string(labelId);

			m_out << "// --- for (" << iterName << ") ---\n";
			CompileExpr(forStmt->startExpr.get());
			m_out << "SET " << iterName << "\n";
			m_currentLocals.emplace_back(iterName);

			CompileExpr(forStmt->endExpr.get());
			m_out << "SET " << limitName << "\n";
			m_currentLocals.emplace_back(limitName);

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
				{
					CompileStatement(s.get());
				}
			}

			m_out << "GET " << iterName << "\n";
			m_out << "CONST 1\n";
			m_out << "ADD\n";
			m_out << "SET " << iterName << "\n";
			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
		else
		{
			throw std::runtime_error("TextCompiler: Unhandled Statement type!");
		}
	}

	void CompileExpr(const ast::Expr* expr)
	{
		if (!expr)
		{
			return;
		}

		if (const auto litExpr = dynamic_cast<const ast::LiteralExpr*>(expr))
		{
			if (litExpr->token.type == TokenType::KwTrue)
			{
				m_out << "CONST 1\n"; // Или m_out << "TRUE\n" если добавите опкоды True/False в ВМ
				return;
			}
			if (litExpr->token.type == TokenType::KwFalse)
			{
				m_out << "CONST 0\n"; // Или m_out << "FALSE\n"
				return;
			}
			m_out << "CONST " << litExpr->token.lexeme << "\n";
			return;
		}

		if (const auto idExpr = dynamic_cast<const ast::IdentifierExpr*>(expr))
		{
			const std::string name = idExpr->name.ToString();

			if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
			{
				m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else if (std::ranges::find(m_currentLocals, name) != m_currentLocals.end())
			{
				m_out << "GET " << name << "\n";
				if (m_functionBoxedVars[m_currentFunction].contains(name))
				{
					m_out << "UNBOX\n";
				}
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
				const std::string name = id->name.ToString();
				if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
				{
					CompileExpr(assignExpr->value.get());
					m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					if (m_functionBoxedVars[m_currentFunction].contains(name))
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
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(callExpr->callee.get()))
			{
				directFuncName = id->name.ToString();
				if (directFuncName == "print" || directFuncName == "make_array" || directFuncName == "len")
				{
					isDirectCall = true;
				}
				else if (std::ranges::find(m_currentLocals, directFuncName) == m_currentLocals.end() && std::ranges::find(m_currentUpvalues, directFuncName) == m_currentUpvalues.end())
				{
					isDirectCall = true;
				}
			}

			if (isDirectCall)
			{
				for (const auto& arg : callExpr->arguments)
				{
					CompileExpr(arg.get());
				}
				if (directFuncName == "print" || directFuncName == "len")
				{
					m_out << "NATIVE \"" << directFuncName << "\" " << callExpr->arguments.size() << "\n";
				}
				else
				{
					m_out << "CALL " << directFuncName << " " << callExpr->arguments.size() << "\n";
				}
			}
			else
			{
				CompileExpr(callExpr->callee.get());
				for (const auto& arg : callExpr->arguments)
				{
					CompileExpr(arg.get());
				}
				m_out << "CALL_INDIRECT " << callExpr->arguments.size() << "\n";
			}
			return;
		}

		if (const auto unExpr = dynamic_cast<const ast::UnaryExpr*>(expr))
		{
			std::string op = unExpr->op.ToString();

			// 1. Простые унарные операторы
			if (op == "-")
			{
				m_out << "CONST 0\n";
				CompileExpr(unExpr->operand.get());
				m_out << "SUB\n";
				return;
			}
			if (op == "+")
			{
				CompileExpr(unExpr->operand.get());
				return;
			}
			if (op == "!")
			{
				CompileExpr(unExpr->operand.get());
				m_out << "CONST 0\n";
				m_out << "EQUAL\n"; // Инверсия: если x было 1, то 1 == 0 -> 0. Идеально!
				return;
			}
			if (op == "~")
			{
				CompileExpr(unExpr->operand.get());
				m_out << "BIT_NOT\n"; // Убедитесь, что в RVM есть опкод BIT_NOT
				return;
			}

			// 2. Инкремент (++) и Декремент (--)
			if (op == "++" || op == "--")
			{
				std::string vmOp = (op == "++") ? "INC" : "DEC";
				std::string revOp = (op == "++") ? "DEC" : "INC"; // Для отката в Postfix

				if (auto id = dynamic_cast<const ast::IdentifierExpr*>(unExpr->operand.get()))
				{
					std::string name = id->name.ToString();
					auto upIt = std::ranges::find(m_currentUpvalues, name);

					// ШАГ 1: Читаем, делаем INC/DEC, сохраняем
					if (upIt != m_currentUpvalues.end())
					{
						m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
						m_out << vmOp << "\n";
						m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
					}
					else if (m_functionBoxedVars[m_currentFunction].contains(name))
					{
						m_out << "GET " << name << "\n";
						m_out << "GET " << name << "\n";
						m_out << "UNBOX\n";
						m_out << vmOp << "\n";
						m_out << "STORE_BOX\n";
					}
					else
					{
						m_out << "GET " << name << "\n";
						m_out << vmOp << "\n";
						m_out << "SET " << name << "\n";
					}

					// ШАГ 2: Возвращаем результат на стек
					if (upIt != m_currentUpvalues.end())
					{
						m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
					}
					else
					{
						m_out << "GET " << name << "\n";
						if (m_functionBoxedVars[m_currentFunction].contains(name))
						{
							m_out << "UNBOX\n";
						}
					}

					// Для постфикса (x++) просто откатываем значение НА СТЕКЕ (в памяти остается новое!)
					if (unExpr->isPostfix)
					{
						m_out << revOp << "\n";
					}
				}
				return;
			}
		}

		if (const auto binExpr = dynamic_cast<const ast::BinaryExpr*>(expr))
		{
			CompileExpr(binExpr->left.get());
			CompileExpr(binExpr->right.get());

			// clang-format off
          	if (const std::string op = binExpr->op.ToString();op == "+")       m_out << "ADD\n";
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