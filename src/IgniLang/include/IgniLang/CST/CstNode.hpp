#pragma once

#include <IgniLang/TokenType.hpp>

#include <fsm/lexer.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <vector>

namespace igni
{

struct CstNode
{
	std::string symbol;

	std::optional<fsm::token<TokenType>> token;

	std::vector<std::unique_ptr<CstNode>> children;

	explicit CstNode(std::string symbol)
		: symbol(std::move(symbol))
	{
	}

	void Print(const int depth = 0) const
	{
		const std::string indent(depth * 2, ' ');
		std::cout << indent << symbol;
		if (token)
		{
			std::cout << " ['" << token->lexeme << "']";
		}
		std::cout << "\n";
		for (const auto& child : children)
		{
			child->Print(depth + 1);
		}
	}
};

} // namespace igni