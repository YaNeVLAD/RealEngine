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
	Identifier,
	Integer,
	Double,
	String,
	Whitespace,
	Unknown
};

const std::unordered_map<re::HashedString, OpCode> s_instructionMap = {
	{ "CONST"_hs, OpCode::Const },
	{ "ADD"_hs, OpCode::Add },
	{ "SUB"_hs, OpCode::Sub },
	{ "MUL"_hs, OpCode::Mul },
	{ "DIV"_hs, OpCode::Div },
	{ "POP"_hs, OpCode::Pop },
	{ "RETURN"_hs, OpCode::Return }
};

std::string ProcessEscapeSequences(std::string_view raw)
{
	std::size_t pos = raw.find('\\');

	if (pos == std::string_view::npos)
	{
		return std::string(raw);
	}

	std::string result;
	result.reserve(raw.length());

	std::size_t start = 0;
	while (pos != std::string_view::npos)
	{
		result.append(raw.data() + start, pos - start);

		if (pos + 1 < raw.length())
		{
			switch (raw[pos + 1])
			{
				// clang-format off
			case 'n':  result.push_back('\n'); break;
			case 't':  result.push_back('\t'); break;
			case 'r':  result.push_back('\r'); break;
			case '"':  result.push_back('"');  break;
			case '\\': result.push_back('\\'); break;
			case '0':  result.push_back('\0'); break;
			default:   result.push_back(raw[pos + 1]); break;
				// clang-format on
			}
			start = pos + 2;
		}
		else
		{
			result.push_back('\\');
			start = pos + 1;
		}

		pos = raw.find('\\', start);
	}

	if (start < raw.length())
	{
		result.append(raw.data() + start, raw.length() - start);
	}

	return result;
}

} // namespace

namespace re::rvm
{

bool Assembler::Compile(const std::string& source, Chunk& outChunk)
{
	m_locals.clear();
	m_localsCount = 0;

	fsm::lexer<TokenType, fsm::std_regex_matcher> lexer(source);
	lexer
		.add_rule(R"([a-zA-Z_][a-zA-Z0-9_]*)", TokenType::Identifier)
		.add_rule(R"([0-9]+\.[0-9]+)", TokenType::Double)
		.add_rule(R"([0-9]+)", TokenType::Integer)
		.add_rule(R"("(?:[^"\\]|\\.)*")", TokenType::String)
		.add_rule(R"([ \t\r\n]+)", TokenType::Whitespace, true);

	std::unordered_map<Hash_t, std::int64_t> labels;
	struct Fixup
	{
		std::size_t codeOffset;
		String funcName;
	};
	std::vector<Fixup> fixups;

	while (const auto tokenOpt = lexer.next())
	{
		if (const auto& token = *tokenOpt; token.type == TokenType::Identifier)
		{
			auto hash = HashedString{ token.lexeme };

			if (auto it = s_instructionMap.find(hash); it != s_instructionMap.end())
			{
				OpCode op = it->second;
				outChunk.Write(static_cast<uint8_t>(op));

				if (op == OpCode::Const)
				{
					const auto& argOpt = lexer.next();
					if (!argOpt)
					{
						return false; // Error
					}
					const auto& arg = *argOpt;
					if (arg.type == TokenType::Integer)
					{
						std::int64_t val = std::stoll(std::string(arg.lexeme));
						outChunk.Write(outChunk.AddConstant(val));
					}
					else if (arg.type == TokenType::Double)
					{
						double val = std::stod(std::string(arg.lexeme));
						outChunk.Write(outChunk.AddConstant(val));
					}
					else if (arg.type == TokenType::String)
					{
						auto rawStr = arg.lexeme.substr(1, arg.lexeme.size() - 2);
						outChunk.Write(outChunk.AddConstant(String(ProcessEscapeSequences(rawStr))));
					}
					else
					{
						std::cerr << "Expected number after CONST\n";
						return false;
					}
				}
			}
			else if (hash == "SET"_hs)
			{
				const auto argOpt = lexer.next();
				if (!argOpt)
				{
					return false;
				}
				const auto& arg = *argOpt;

				if (arg.type != TokenType::Identifier)
				{
					std::cerr << "Expected variable name after SET\n";
					return false;
				}

				std::string varName(arg.lexeme);
				const std::uint8_t slot = DeclareVariable(varName);

				outChunk.Write(static_cast<uint8_t>(OpCode::SetLocal));
				outChunk.Write(slot);
			}
			else if (hash == "GET"_hs)
			{
				const auto argOpt = lexer.next();
				if (!argOpt)
				{
					return false;
				}
				const auto& arg = *argOpt;

				if (arg.type != TokenType::Identifier)
				{
					std::cerr << "Expected variable name after GET\n";
					return false;
				}

				std::string varName(arg.lexeme);
				auto slotOpt = ResolveVariable(varName);

				if (!slotOpt.has_value())
				{
					std::cerr << "Undefined variable: " << varName << "\n";
					return false;
				}

				outChunk.Write(static_cast<std::uint8_t>(OpCode::GetLocal));
				outChunk.Write(slotOpt.value());
			}
			else if (hash == "DEF"_hs)
			{
				const auto funcNameOpt = lexer.next();
				if (!funcNameOpt.has_value())
				{
					return false;
				}
				const auto& funcName = String(funcNameOpt->lexeme);
				auto hs = funcName.Hash();
				labels[hs] = outChunk.Size();
			}
			else if (hash == "CALL"_hs)
			{
				const auto funcNameOpt = lexer.next();
				if (!funcNameOpt.has_value())
				{
					return false;
				}
				const auto& funcName = String(funcNameOpt->lexeme);

				outChunk.Write(static_cast<std::uint8_t>(OpCode::Call));
				fixups.push_back({ outChunk.Size(), funcName });
				outChunk.Write(0);
			}
			else if (hash == "NATIVE"_hs)
			{
				const auto funcNameOpt = lexer.next();
				if (!funcNameOpt.has_value())
				{
					return false;
				}
				const auto argsOpt = lexer.next();
				if (!argsOpt.has_value())
				{
					return false;
				}
				auto rawStr = funcNameOpt->lexeme.substr(1, funcNameOpt->lexeme.size() - 2);
				const auto funcName = String(ProcessEscapeSequences(rawStr));

				const auto& argCount = static_cast<std::uint8_t>(std::stoul(std::string(argsOpt->lexeme)));

				outChunk.Write(static_cast<std::uint8_t>(OpCode::Native));
				outChunk.Write(outChunk.AddConstant(funcName));
				outChunk.Write(argCount);
			}
			else
			{
				std::cerr << "Unknown instruction: " << token.lexeme << "\n";

				return false;
			}
		}
		else
		{
			std::cerr << "Unexpected token: " << token.lexeme << "\n";

			return false;
		}
	}

	for (const auto& [codeOffset, funcName] : fixups)
	{
		auto hash = funcName.Hash();
		if (!labels.contains(hash))
		{
			std::cerr << "Error: Undefined function '" << funcName << "'\n";

			return false;
		}

		std::int64_t offset = labels[hash];
		std::uint8_t constIdx = outChunk.AddConstant(offset);

		outChunk.Patch(codeOffset, constIdx);
	}

	return true;
}

std::uint8_t Assembler::DeclareVariable(const std::string& name)
{
	const auto hash = HashedString{ name };
	if (!m_locals.contains(hash))
	{
		m_locals[hash] = m_localsCount++;
	}

	return m_locals[hash];
}

std::optional<std::uint8_t> Assembler::ResolveVariable(const std::string& name)
{
	const auto hash = HashedString{ name };
	if (m_locals.contains(hash))
	{
		return m_locals[hash];
	}

	return std::nullopt;
}

} // namespace re::rvm