#pragma once

#include "TokenType.hpp"

#include <fsm/lexer.hpp>

#include <string_view>

namespace igni
{

using ScriptLexer = fsm::lexer<TokenType, fsm::std_regex_matcher>;

inline ScriptLexer CreateLexer(const std::string_view source = "")
{
	ScriptLexer lexer(source);

	lexer.add_rule(R"([ \t\r\n]+)", TokenType::Error, true);
	lexer.add_rule(R"(//[^\n]*)", TokenType::Error, true);

#define DEFINE_LEXER_RULE(name, regex, grammar_term) \
	lexer.add_rule(regex, TokenType::name);

	SCRIPT_TOKENS(DEFINE_LEXER_RULE)
#undef DEFINE_LEXER_RULE

	return lexer;
}

} // namespace igni