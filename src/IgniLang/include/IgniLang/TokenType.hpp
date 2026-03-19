#pragma once

#include <string>

namespace igni
{
// clang-format off
#define SCRIPT_TOKENS(TOKEN) \
    /* Keywords */ \
    TOKEN(KwPackage,    R"(package\b)",        "package") \
    TOKEN(KwImport,     R"(import\b)",         "import") \
    TOKEN(KwClass,      R"(class\b)",          "class") \
    TOKEN(KwFun,        R"(fun\b)",            "fun") \
    TOKEN(KwVar,        R"(var\b)",            "var") \
    TOKEN(KwVal,        R"(val\b)",            "val") \
    TOKEN(KwIf,         R"(if\b)",             "if") \
    TOKEN(KwElse,       R"(else\b)",           "else") \
    TOKEN(KwWhile,      R"(while\b)",          "while") \
    TOKEN(KwFor,        R"(for\b)",            "for") \
    TOKEN(KwIn,         R"(in\b)",             "in") \
    TOKEN(KwReturn,     R"(return\b)",         "return") \
    TOKEN(KwBreak,      R"(break\b)",          "break") \
    TOKEN(KwPublic,     R"(public\b)",         "public") \
    TOKEN(KwPrivate,    R"(private\b)",        "private") \
    TOKEN(KwInternal,   R"(internal\b)",       "internal") \
    TOKEN(KwExternal,   R"(external\b)",       "external") \
    TOKEN(KwCompileTime,R"(compile_time\b)",   "compile_time") \
    TOKEN(KwTrue,       R"(true\b)",           "true") \
    TOKEN(KwFalse,      R"(false\b)",          "false") \
    TOKEN(KwNull,       R"(null\b)",           "null") \
    /* Literals */ \
    TOKEN(FloatConst,   R"([0-9]+\.[0-9]+)",   "floatCon") \
    TOKEN(IntConst,     R"([0-9]+)",           "intCon") \
    TOKEN(StringConst,  R"("[^"]*")",          "stringCon") \
    TOKEN(Identifier,   R"([a-zA-Z_]\w*)",     "ident") \
    /* Double char operations */ \
    TOKEN(Eq,           R"(==)",               "==") \
    TOKEN(Neq,          R"(!=)",               "!=") \
    TOKEN(LessEq,       R"(<=)",               "<=") \
    TOKEN(GreaterEq,    R"(>=)",               ">=") \
    TOKEN(And,          R"(&&)",               "&&") \
    TOKEN(Or,           R"(\|\|)",             "||") \
    TOKEN(Range,        R"(\.\.)",             "..") \
    TOKEN(QuestionDot,  R"(\?\.)",             "?.") \
    /* Single char operations */ \
    TOKEN(Assign,       R"(=)",                "=") \
    TOKEN(Less,         R"(<)",                "<") \
    TOKEN(Greater,      R"(>)",                ">") \
    TOKEN(Plus,         R"(\+)",               "+") \
    TOKEN(Minus,        R"(\-)",               "-") \
    TOKEN(Star,         R"(\*)",               "*") \
    TOKEN(Slash,        R"(/)",                "/") \
    TOKEN(Mod,          R"(%)",                "%") \
    TOKEN(Dot,          R"(\.)",               ".") \
    TOKEN(Question,     R"(\?)",               "?") \
    TOKEN(Comma,        R"(,)",                ",") \
    TOKEN(Colon,        R"(:)",                ":") \
    TOKEN(Semicolon,    R"(;)",                ";") \
    TOKEN(At,           R"(@)",                "@") \
    /* Brackets */ \
    TOKEN(LParen,       R"(\()",               "(") \
    TOKEN(RParen,       R"(\))",               ")") \
    TOKEN(LBrace,       R"(\{)",               "{") \
    TOKEN(RBrace,       R"(\})",               "}") \
    TOKEN(LBracket,     R"(\[)",               "[") \
    TOKEN(RBracket,     R"(\])",               "]")
// clang-format on

enum class TokenType
{
	EndOfFile,
	Error,

#define DEFINE_ENUM(name, regex, grammar_term) name,
	SCRIPT_TOKENS(DEFINE_ENUM)
#undef DEFINE_ENUM
};

inline std::string ToTerminalString(const TokenType type)
{
	switch (type)
	{
#define DEFINE_STRING_MAPPING(name, regex, grammar_term) \
	case TokenType::name:                                \
		return grammar_term;
		SCRIPT_TOKENS(DEFINE_STRING_MAPPING)
#undef DEFINE_STRING_MAPPING
	case TokenType::EndOfFile:
		return "$";
	default:
		return "UNKNOWN";
	}
}

} // namespace igni