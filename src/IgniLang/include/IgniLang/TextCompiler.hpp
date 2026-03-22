#pragma once

#include "AstNodes.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace igni::compiler
{

class TextCompiler
{
public:
	std::string Compile(const ast::Program* program)
	{
		m_out.str("");
		m_out.clear();
		m_currentLocals.clear();
		m_labelCount = 0;

		m_out << "// ==========================================\n";
		m_out << "// Auto-generated RVM Assembly\n";
		m_out << "// Package: " << program->packageName.ToString() << "\n";
		m_out << "// ==========================================\n\n";

		m_out << "// --- Entry Point ---\n";
		m_out << "CALL main 0\n"; // Вызываем главную функцию
		m_out << "RETURN\n\n"; // Корректно глушим ВМ после выхода из main
		// =================================

		for (const auto& stmt : program->statements)
		{
			if (stmt)
			{
				CompileStatement(stmt.get());
				m_out << "\n";
			}
		}

		return m_out.str();
	}

private:
	int m_labelCount = 0;
	std::stringstream m_out;
	std::vector<std::string> m_currentLocals;

	bool IsLocalVariable(const std::string& name) const
	{
		return std::ranges::find(m_currentLocals, name) != m_currentLocals.end();
	}

	void CompileStatement(const ast::Statement* stmt)
	{
		if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt))
		{
			m_out << "// val " << valDecl->name.ToString() << "\n";
			if (valDecl->initializer)
				CompileExpr(valDecl->initializer.get());
			else
				m_out << "CONST 0\n";
			m_out << "SET " << valDecl->name.ToString() << "\n";
			m_currentLocals.emplace_back(valDecl->name.ToString()); // Регистрируем локальную переменную
		}
		else if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt))
		{
			m_out << "// var " << varDecl->name.ToString() << "\n";
			if (varDecl->initializer)
				CompileExpr(varDecl->initializer.get());
			else
				m_out << "CONST 0\n";
			m_out << "SET " << varDecl->name.ToString() << "\n";
			m_currentLocals.emplace_back(varDecl->name.ToString()); // Регистрируем локальную переменную
		}
		else if (const auto exprStmt = dynamic_cast<const ast::ExprStmt*>(stmt))
		{
			CompileExpr(exprStmt->expr.get());
			// Инструкции-выражения (например, `foo();`) обязаны очищать стек!
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
			m_out << "FUN " << funDecl->name.ToString() << "\n";

			const std::vector<std::string> previousLocals = m_currentLocals;
			m_currentLocals.clear();

			// Забираем аргументы из стека в переменные (в обратном порядке)
			for (auto it = funDecl->parameters.rbegin(); it != funDecl->parameters.rend(); ++it)
			{
				std::string paramName = it->ToString();
				m_out << "SET " << it->ToString() << "\n";
				m_currentLocals.emplace_back(paramName);
			}

			if (funDecl->body)
			{
				for (const auto& s : funDecl->body->statements)
					CompileStatement(s.get());
			}

			m_out << "CONST 0\nRETURN\n";

			m_currentLocals = previousLocals;
		}
		else if (const auto ifStmt = dynamic_cast<const ast::IfStmt*>(stmt))
		{
			// Генерируем уникальные метки для этого if
			const int currentLabelId = m_labelCount++;
			const std::string elseLabel = "L_else_" + std::to_string(currentLabelId);
			const std::string endLabel = "L_end_" + std::to_string(currentLabelId);

			// 1. Вычисляем условие (положит 1 или 0 на стек)
			m_out << "// --- if ---\n";
			CompileExpr(ifStmt->condition.get());

			// 2. Условный прыжок
			if (ifStmt->elseBranch)
			{
				m_out << "JMP_IF_FALSE " << elseLabel << "\n";
			}
			else
			{
				m_out << "JMP_IF_FALSE " << endLabel << "\n";
			}

			// 3. Ветка Then
			if (ifStmt->thenBranch)
			{
				for (const auto& s : ifStmt->thenBranch->statements)
				{
					CompileStatement(s.get());
				}
			}

			// 4. Ветка Else (если есть)
			if (ifStmt->elseBranch)
			{
				// Если Then-ветка отработала, нужно перепрыгнуть Else
				m_out << "JMP " << endLabel << "\n";
				m_out << "LABEL " << elseLabel << "\n";

				// ElseBranch может быть блоком или другим ifStmt (else if)
				CompileStatement(ifStmt->elseBranch.get());
			}

			// 5. Конец условия
			m_out << "LABEL " << endLabel << "\n";
		}
		else if (const auto blockStmt = dynamic_cast<const ast::Block*>(stmt))
		{
			// Добавим обработку "голого" блока, так как ElseBranch может быть блоком
			for (const auto& s : blockStmt->statements)
			{
				CompileStatement(s.get());
			}
		}
		else if (const auto whileStmt = dynamic_cast<const ast::WhileStmt*>(stmt))
		{
			int labelId = m_labelCount++;
			std::string startLabel = "L_while_start_" + std::to_string(labelId);
			std::string endLabel = "L_while_end_" + std::to_string(labelId);

			m_out << "LABEL " << startLabel << "\n";

			// Условие продолжения
			CompileExpr(whileStmt->condition.get());
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			if (whileStmt->body)
			{
				// Если мы поддержим break в будущем, нужно будет передавать endLabel внутрь
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
			std::string limitName = "_for_limit_" + std::to_string(labelId); // Скрытая переменная предела

			// 1. Инициализация (var i = startExpr)
			m_out << "// --- for (" << iterName << ") ---\n";
			CompileExpr(forStmt->startExpr.get());
			m_out << "SET " << iterName << "\n";
			m_currentLocals.push_back(iterName);

			// 2. Кешируем предел, чтобы не вычислять его на каждой итерации
			CompileExpr(forStmt->endExpr.get());
			m_out << "SET " << limitName << "\n";
			m_currentLocals.push_back(limitName);

			// 3. Метка старта
			m_out << "LABEL " << startLabel << "\n";

			// 4. Условие (i <= limit). В RVM есть только LESS.
			// Если limit < i (т.е. переборщили), это 1. Иначе 0.
			// Нам нужно прыгнуть в конец, если это 1.
			// Инвертируем: 1 - (limit < i). Если переборщили, результат 0 -> JMP_IF_FALSE прыгнет!
			m_out << "CONST 1\n";
			m_out << "GET " << limitName << "\n";
			m_out << "GET " << iterName << "\n";
			m_out << "LESS\n";
			m_out << "SUB\n";
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			// 5. Тело цикла
			if (forStmt->body)
			{
				for (const auto& s : forStmt->body->statements)
					CompileStatement(s.get());
			}

			// 6. Инкремент (i = i + 1)
			m_out << "GET " << iterName << "\n";
			m_out << "CONST 1\n";
			m_out << "ADD\n";
			m_out << "SET " << iterName << "\n";

			// 7. Возврат к проверке условия
			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
	}

	void CompileExpr(const ast::Expr* expr)
	{
		if (!expr)
			return;

		// --- ЛИТЕРАЛЫ ---
		if (const auto litExpr = dynamic_cast<const ast::LiteralExpr*>(expr))
		{
			if (litExpr->token.type == TokenType::IntConst || litExpr->token.type == TokenType::FloatConst)
			{
				m_out << "CONST " << litExpr->token.lexeme << "\n";
			}
			else if (litExpr->token.type == TokenType::KwTrue)
			{
				m_out << "CONST 1\n";
			}
			else if (litExpr->token.type == TokenType::KwFalse)
			{
				m_out << "CONST 0\n";
			}
			else if (litExpr->token.type == TokenType::StringConst)
			{
				m_out << "CONST " << litExpr->token.lexeme << "\n";
			}
			return;
		}

		// --- ИДЕНТИФИКАТОРЫ ---
		if (const auto idExpr = dynamic_cast<const ast::IdentifierExpr*>(expr))
		{
			const std::string name = idExpr->name.ToString();

			if (IsLocalVariable(name))
			{
				m_out << "GET " << name << "\n"; // Читаем значение
			}
			else
			{
				// Если переменной нет, значит мы обращаемся к глобальной функции как к значению!
				m_out << "LOAD_FUN " << name << "\n";
			}

			return;
		}

		// --- ВЫЗОВЫ ---
		if (const auto callExpr = dynamic_cast<const ast::CallExpr*>(expr))
		{
			std::string directFuncName = "";
			bool isDirectCall = false;

			// Проверяем, пытаемся ли мы вызвать напрямую по имени
			if (auto id = dynamic_cast<const ast::IdentifierExpr*>(callExpr->callee.get()))
			{
				directFuncName = id->name.ToString();
				// Если это нативная функция или глобальная функция (не локальная переменная)
				if (directFuncName == "print" || directFuncName == "read" || !IsLocalVariable(directFuncName))
				{
					isDirectCall = true;
				}
			}

			if (isDirectCall)
			{
				// ПРЯМОЙ ВЫЗОВ (Direct Call)
				for (const auto& arg : callExpr->arguments)
					CompileExpr(arg.get());

				if (directFuncName == "print" || directFuncName == "read")
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
				// КОСВЕННЫЙ ВЫЗОВ (Indirect Call)
				// 1. Вычисляем саму функцию (кладем адрес на стек)
				CompileExpr(callExpr->callee.get());

				// 2. Кладем аргументы
				for (const auto& arg : callExpr->arguments)
					CompileExpr(arg.get());

				// 3. Вызываем
				m_out << "CALL_INDIRECT " << callExpr->arguments.size() << "\n";
			}
			return;
		}

		// --- БИНАРНЫЕ ОПЕРАЦИИ ---
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
			else if (op == "&&") m_out << "AND\n"; // Если поддерживаете в ВМ
			else if (op == "||") m_out << "OR\n"; // Если поддерживаете в ВМ
			// clang-format on
			else
			{
				throw std::runtime_error("TextCompiler: Unsupported binary operator '" + op + "'");
			}

			return;
		}

		std::cerr << "Warning: Unhandled expression type in TextCompiler.\n";
	}
};

} // namespace igni::compiler