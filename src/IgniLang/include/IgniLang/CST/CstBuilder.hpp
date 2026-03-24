#pragma once

#include <IgniLang/CST/CstNode.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <fsm/slr/parser.hpp>

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace igni
{

class CstBuilder
{
public:
	CstBuilder(fsm::slr::parser<re::String>& parser, re::String epsilonSymbol)
		: m_parser(parser)
		, m_epsilon(std::move(epsilonSymbol))
	{
	}

	std::unique_ptr<CstNode> Build(const std::vector<fsm::token<TokenType>>& tokens)
	{
		std::vector<re::String> parserInput;
		parserInput.reserve(tokens.size());
		for (const auto& token : tokens)
		{
			parserInput.emplace_back(ToTerminalString(token.type));
		}

		m_parser.begin(parserInput);

		std::vector<std::unique_ptr<CstNode>> nodeStack;
		std::size_t tokenIndex = 0;
		bool finished = false;
		while (!finished)
		{
			auto event = m_parser.next();

			fsm::utility::overloaded_visitor(
				*event,
				[&](const fsm::slr::event_shift<re::String>& e) {
					auto leaf = std::make_unique<CstNode>(e.token);

					if (tokenIndex < tokens.size())
					{
						leaf->token = tokens[tokenIndex];
						tokenIndex++;
					}

					nodeStack.push_back(std::move(leaf));
				},
				[&](const fsm::slr::event_reduce<re::String>& e) {
					auto parent = std::make_unique<CstNode>(e.rule.lhs);

					std::size_t popCount = e.rule.rhs.size();
					if (popCount == 1 && e.rule.rhs[0] == m_epsilon)
					{
						popCount = 0;
					}

					parent->children.resize(popCount);
					for (std::size_t i = 0; i < popCount; ++i)
					{
						parent->children[popCount - 1 - i] = std::move(nodeStack.back());
						nodeStack.pop_back();
					}

					if (popCount == 0)
					{
						parent->children.push_back(std::make_unique<CstNode>(m_epsilon));
					}

					nodeStack.push_back(std::move(parent));
				},
				[&](const fsm::slr::event_accept&) {
					std::cout << "CST generation completed successfully" << std::endl;
					finished = true;
				},
				[&](const fsm::slr::event_error<re::String>& e) {
					const auto errToken = (tokenIndex < tokens.size()) ? tokens[tokenIndex] : fsm::token<TokenType>{};
					std::cerr << "[Syntax Error] Unexpected token '" << e.unexpected_token.ToString()
							  << "' at line " << errToken.line;
					std::cerr << ". Expected: ";
					for (const auto& token : e.expected_tokens)
					{
						std::cerr << token.ToString() << ", ";
					}
					std::cerr << std::endl;

					finished = true;
					nodeStack.clear();
				});
		}

		if (!nodeStack.empty())
		{
			return std::move(nodeStack.back());
		}

		return nullptr;
	}

private:
	fsm::slr::parser<re::String>& m_parser;
	re::String m_epsilon;
};

} // namespace igni