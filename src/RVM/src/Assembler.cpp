#include <RVM/Assembler.hpp>

#include <Core/HashedString.hpp>

#include <fsm/lexer.hpp>

#include <iostream>

namespace
{

using namespace re::rvm;
using namespace re::literals;

enum class TokenType
{
	Instruction,
	Integer,
	Double,
	Whitespace,
	Unknown
};

const std::unordered_map<re::HashedString, OpCode> s_instructionMap = {
	{ "CONST"_hs, OpCode::Const },
	{ "ADD"_hs, OpCode::Add },
	{ "SUB"_hs, OpCode::Sub },
	{ "MUL"_hs, OpCode::Mul },
	{ "DIV"_hs, OpCode::Div },
	{ "RETURN"_hs, OpCode::Return }
};

} // namespace

namespace re::rvm
{

bool Assembler::Compile(const std::string& source, Chunk& outChunk)
{
	fsm::lexer<TokenType, fsm::std_regex_matcher> lexer(source);
	lexer
		.add_rule(R"([A-Z]+)", TokenType::Instruction)
		.add_rule(R"([0-9]+\.[0-9]+)", TokenType::Double)
		.add_rule(R"([0-9]+)", TokenType::Integer)
		.add_rule(R"([ \t\r\n]+)", TokenType::Whitespace, true);

	while (auto tokenOpt = lexer.next())
	{
		switch (const auto& token = *tokenOpt; token.type)
		{
		case TokenType::Instruction: {
			HashedString hash{ token.lexeme };

			auto it = s_instructionMap.find(hash);
			if (it == s_instructionMap.end())
			{
				std::cerr << "Error: Unknown instruction '" << token.lexeme << "' at line " << token.line << "\n";
				return false;
			}

			OpCode op = it->second;
			outChunk.Write(static_cast<uint8_t>(op));

			if (op == OpCode::Const)
			{
				auto argTokenOpt = lexer.next();

				if (!argTokenOpt)
				{
					std::cerr << "Error: Unexpected end of file after CONST\n";
					return false;
				}

				if (const auto& argToken = *argTokenOpt; argToken.type == TokenType::Integer)
				{
					Int val = std::stoi(std::string(argToken.lexeme));
					const std::uint8_t idx = outChunk.AddConstant(val);
					outChunk.Write(idx);
				}
				else if (argToken.type == TokenType::Double)
				{
					Double val = std::stod(std::string(argToken.lexeme));
					const std::uint8_t idx = outChunk.AddConstant(val);
					outChunk.Write(idx);
				}
				else
				{
					std::cerr << "Error: Expected number after CONST, got '" << argToken.lexeme << "'\n";
					return false;
				}
			}
			break;
		}

		case TokenType::Integer:
		case TokenType::Double:
			std::cerr << "Error: Unexpected number '" << token.lexeme << "' without instruction\n";
			return false;

		default:
			std::cerr << "Error: Unexpected token '" << token.lexeme << "'\n";
			return false;
		}
	}

	return true;
}

} // namespace re::rvm