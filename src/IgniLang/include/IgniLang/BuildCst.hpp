#pragma once

#include <IgniLang/CstNode.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <fsm/ll1/table.hpp>

namespace igni
{

inline std::unique_ptr<CstNode> BuildCst(
	const fsm::ll1::table<std::string>& tbl,
	const std::string& start_symbol,
	const std::vector<fsm::token<TokenType>>& tokens)
{
	const auto root = std::make_unique<CstNode>("__ROOT__");

	struct StackItem
	{
		std::string symbol;
		CstNode* parent;
	};

	const auto& endMarker = tbl.end_marker();
	const auto& epsilon = tbl.epsilon();

	std::vector<StackItem> stack;
	stack.emplace_back(endMarker, root.get());
	stack.emplace_back(start_symbol, root.get());

	size_t inputPtr = 0;

	while (!stack.empty())
	{
		auto [topSymbol, parent] = stack.back();
		stack.pop_back();

		auto newNode = std::make_unique<CstNode>(topSymbol);
		CstNode* currentNode = newNode.get();

		parent->children.push_back(std::move(newNode));

		const bool isEof = (inputPtr >= tokens.size());
		auto currentToken = isEof ? fsm::token{ TokenType::EndOfFile, "" } : tokens[inputPtr];
		std::string currentTerminal = ToTerminalString(currentToken.type);

		if (topSymbol == epsilon)
		{
			continue;
		}

		if (!tbl.entries().contains(topSymbol))
		{
			if (topSymbol == currentTerminal || topSymbol == endMarker)
			{
				currentNode->token = currentToken;

				if (!isEof)
				{
					inputPtr++;
				}
			}
			else
			{
				std::cerr << "[Syntax Error] Expected '" << topSymbol
						  << "', got '" << currentTerminal << "' at line "
						  << currentToken.line << "\n";
				return nullptr;
			}
		}
		else
		{
			if (tbl.has_rule(topSymbol, currentTerminal))
			{
				const auto& [lhs, rhs] = tbl.at(topSymbol, currentTerminal);

				for (auto it = rhs.rbegin(); it != rhs.rend(); ++it)
				{
					stack.emplace_back(*it, currentNode);
				}
			}
			else
			{
				std::cerr << "[Syntax Error] Unexpected token '" << currentTerminal
						  << "' at line " << currentToken.line << "\n";
				return nullptr;
			}
		}
	}

	return std::move(root->children.front());
}

inline std::unique_ptr<CstNode> BuildCst(
	const fsm::ll1::table<std::string>& tbl,
	const std::string& start_symbol,
	ScriptLexer& lexer)
{
	const auto root = std::make_unique<CstNode>("__ROOT__");

	struct StackItem
	{
		std::string symbol;
		CstNode* parent;
	};

	const auto& endMarker = tbl.end_marker();
	const auto& epsilon = tbl.epsilon();

	std::vector<StackItem> stack;
	stack.emplace_back(endMarker, root.get());
	stack.emplace_back(start_symbol, root.get());

	while (!stack.empty())
	{
		auto [topSymbol, parent] = stack.back();
		stack.pop_back();

		auto newNode = std::make_unique<CstNode>(topSymbol);
		CstNode* currentNode = newNode.get();
		parent->children.push_back(std::move(newNode));

		if (topSymbol == epsilon)
		{
			continue;
		}

		const auto currentToken = lexer.peek();
		if (!currentToken)
		{
			continue;
		}

		std::string currentTerminal = ToTerminalString(currentToken->type);

		if (!tbl.entries().contains(topSymbol))
		{
			if (topSymbol == currentTerminal || (topSymbol == endMarker && currentToken->type == TokenType::EndOfFile))
			{
				currentNode->token = currentToken;
				lexer.next();
			}
			else
			{
				std::cerr << "[Syntax Error] Expected '" << topSymbol
						  << "', but found '" << currentTerminal
						  << "' at line " << currentToken->line << "\n";
				return nullptr;
			}
		}
		else
		{
			if (tbl.has_rule(topSymbol, currentTerminal))
			{
				const auto& [lhs, rhs] = tbl.at(topSymbol, currentTerminal);

				for (auto it = rhs.rbegin(); it != rhs.rend(); ++it)
				{
					stack.push_back({ *it, currentNode });
				}
			}
			else
			{
				std::cerr << "[Syntax Error] No rule for '" << topSymbol
						  << "' with token '" << currentTerminal
						  << "' at line " << currentToken->line << "\n";
				return nullptr;
			}
		}
	}

	return std::move(root->children.front());
}

} // namespace igni