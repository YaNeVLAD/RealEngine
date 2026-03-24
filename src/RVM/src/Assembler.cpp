#include <RVM/Assembler.hpp>

#include <Core/HashedString.hpp>
#include <Core/String.hpp>

#include <fsm/lexer.hpp>

#include <iostream>

namespace
{

using namespace re::rvm;
using namespace re::literals;

enum class TokenType
{
	Identifier,
	Integer,
	Double,
	String,
	Comment,
	Whitespace,
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

constexpr std::uint8_t PATCH_VALUE = 0;

} // namespace

namespace re::rvm
{

bool Assembler::Compile(const std::string& source, Chunk& outChunk)
{
	struct Fixup
	{
		std::size_t codeOffset;
		String funcName;
	};

	m_locals.clear();
	m_localsCount = 0;

	fsm::lexer<TokenType, fsm::std_regex_matcher> lexer(source);
	lexer
		.add_rule(R"([a-zA-Z_][a-zA-Z0-9_]*)", TokenType::Identifier)
		.add_rule(R"(\-*[0-9]+\.[0-9]+)", TokenType::Double)
		.add_rule(R"(\-*[0-9]+)", TokenType::Integer)
		.add_rule(R"("(?:[^"\\]|\\.)*")", TokenType::String)
		.add_rule("//.*", TokenType::Comment, true)
		.add_rule(R"([ \t\r\n]+)", TokenType::Whitespace, true);

	std::unordered_map<Hash_t, std::int64_t> labels;
	std::vector<Fixup> fixups;

	auto parseConst = [&]() -> bool {
		outChunk.Write(static_cast<uint8_t>(OpCode::Const));

		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}

		if (const auto& arg = *argOpt; arg.type == TokenType::Integer)
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
			const auto rawStr = arg.lexeme.substr(1, arg.lexeme.size() - 2);
			outChunk.Write(outChunk.AddConstant(String(ProcessEscapeSequences(rawStr))));
		}
		else
		{
			std::cerr << "Expected number or string after CONST\n";
			return false;
		}

		return true;
	};

	auto parseSet = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}

		if (argOpt->type != TokenType::Identifier)
		{
			std::cerr << "Expected variable name after SET\n";
			return false;
		}

		const std::string varName(argOpt->lexeme);
		outChunk.Write(static_cast<uint8_t>(OpCode::SetLocal));
		outChunk.Write(DeclareVariable(varName));

		return true;
	};

	auto parseGet = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}

		if (argOpt->type != TokenType::Identifier)
		{
			std::cerr << "Expected variable name after GET\n";
			return false;
		}

		const std::string varName(argOpt->lexeme);
		const auto slotOpt = ResolveVariable(varName);
		if (!slotOpt)
		{
			std::cerr << "Undefined variable: " << varName << "\n";
			return false;
		}

		outChunk.Write(static_cast<std::uint8_t>(OpCode::GetLocal));
		outChunk.Write(slotOpt.value());

		return true;
	};

	auto parseDef = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		if (!funcNameOpt)
		{
			return false;
		}

		m_locals.clear();
		m_localsCount = 0;

		labels[String(funcNameOpt->lexeme).Hash()] = outChunk.Size();

		return true;
	};

	auto parseCall = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		if (!funcNameOpt)
		{
			return false;
		}

		const auto argCountOpt = lexer.next();
		if (!argCountOpt || argCountOpt->type != TokenType::Integer)
		{
			return false;
		}
		const std::uint8_t argCount = static_cast<std::uint8_t>(std::stoul(std::string(argCountOpt->lexeme)));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::Call));
		fixups.push_back({ outChunk.Size(), String(funcNameOpt->lexeme) });
		outChunk.Write(PATCH_VALUE);
		outChunk.Write(argCount);

		return true;
	};

	auto parseNative = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		if (!funcNameOpt)
		{
			return false;
		}
		const auto argsOpt = lexer.next();
		if (!argsOpt)
		{
			return false;
		}

		const auto rawStr = funcNameOpt->lexeme.substr(1, funcNameOpt->lexeme.size() - 2);
		const auto funcName = String(ProcessEscapeSequences(rawStr));
		const auto argCount = static_cast<std::uint8_t>(std::stoul(std::string(argsOpt->lexeme)));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::Native));
		outChunk.Write(outChunk.AddConstant(funcName));
		outChunk.Write(argCount);

		return true;
	};

	auto parseLabel = [&]() -> bool {
		const auto nameOpt = lexer.next();
		if (!nameOpt)
		{
			return false;
		}
		labels[String(nameOpt->lexeme).Hash()] = outChunk.Size();

		return true;
	};

	auto parseJump = [&](OpCode op) -> bool {
		const auto nameOpt = lexer.next();
		if (!nameOpt)
		{
			return false;
		}
		outChunk.Write(static_cast<std::uint8_t>(op));
		fixups.push_back({ outChunk.Size(), String(nameOpt->lexeme) });
		outChunk.Write(PATCH_VALUE);

		return true;
	};

	auto parseCallIndirect = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}
		outChunk.Write(static_cast<uint8_t>(OpCode::CallIndirect));
		outChunk.Write(static_cast<std::uint8_t>(std::stoul(std::string(argOpt->lexeme))));

		return true;
	};

	auto parseNew = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}

		const auto rawStr = argOpt->lexeme.substr(1, argOpt->lexeme.size() - 2);

		outChunk.Write(static_cast<uint8_t>(OpCode::New));
		outChunk.Write(outChunk.AddConstant(String(ProcessEscapeSequences(rawStr))));

		return true;
	};

	auto parseGetProperty = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}
		const auto rawStr = argOpt->lexeme.substr(1, argOpt->lexeme.size() - 2);

		outChunk.Write(static_cast<uint8_t>(OpCode::GetProperty));
		outChunk.Write(outChunk.AddConstant(String(ProcessEscapeSequences(rawStr))));
		return true;
	};

	auto parseSetProperty = [&]() -> bool {
		const auto argOpt = lexer.next();
		if (!argOpt)
		{
			return false;
		}
		const auto rawStr = argOpt->lexeme.substr(1, argOpt->lexeme.size() - 2);

		outChunk.Write(static_cast<uint8_t>(OpCode::SetProperty));
		outChunk.Write(outChunk.AddConstant(String(ProcessEscapeSequences(rawStr))));
		return true;
	};

	auto parseType = [&]() -> bool {
		const auto classNameOpt = lexer.next();
		if (!classNameOpt || classNameOpt->type != TokenType::String)
		{
			std::cerr << "Expected string literal for class name\n";
			return false;
		}

		const auto rawClassStr = classNameOpt->lexeme.substr(1, classNameOpt->lexeme.size() - 2);
		String className(ProcessEscapeSequences(rawClassStr));

		std::vector<String> fields;
		while (const auto tokenOpt = lexer.next())
		{
			const auto& token = *tokenOpt;
			if (auto hash = HashedString(token.lexeme); hash == "END_TYPE"_hs)
			{
				break;
			}
			else if (hash == "FIELD"_hs)
			{
				const auto fieldNameOpt = lexer.next();
				if (!fieldNameOpt || fieldNameOpt->type != TokenType::String)
				{
					std::cerr << "Expected string literal for field name\n";
					return false;
				}
				const auto rawFieldStr = fieldNameOpt->lexeme.substr(1, fieldNameOpt->lexeme.size() - 2);
				fields.emplace_back(ProcessEscapeSequences(rawFieldStr));
			}
			else
			{
				std::cerr << "Unexpected token inside TYPE definition: " << token.lexeme << "\n";
				return false;
			}
		}

		for (auto it = fields.rbegin(); it != fields.rend(); ++it)
		{
			outChunk.Write(static_cast<std::uint8_t>(OpCode::Const));
			outChunk.Write(outChunk.AddConstant(*it));
		}

		outChunk.Write(static_cast<std::uint8_t>(OpCode::Const));
		outChunk.Write(outChunk.AddConstant(className));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::Const));
		outChunk.Write(outChunk.AddConstant(static_cast<Int>(fields.size())));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::DefType));

		return true;
	};

	auto parseMakeClosure = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		if (!funcNameOpt)
		{
			return false;
		}
		const auto argCountOpt = lexer.next();
		if (!argCountOpt || argCountOpt->type != TokenType::Integer)
		{
			return false;
		}

		const std::uint8_t upvCount = static_cast<std::uint8_t>(std::stoul(std::string(argCountOpt->lexeme)));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::MakeClosure));
		fixups.push_back({ outChunk.Size(), String(funcNameOpt->lexeme) });
		outChunk.Write(0);
		outChunk.Write(upvCount);

		return true;
	};

	auto parseLoadNative = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		const auto argsOpt = lexer.next();
		if (!funcNameOpt || !argsOpt || funcNameOpt->type != TokenType::String)
		{
			return false;
		}

		const auto rawStr = funcNameOpt->lexeme.substr(1, funcNameOpt->lexeme.size() - 2);
		const auto funcName = String(ProcessEscapeSequences(rawStr));
		const auto argCount = static_cast<std::uint8_t>(std::stoul(std::string(argsOpt->lexeme)));

		outChunk.Write(static_cast<std::uint8_t>(OpCode::LoadNative));
		outChunk.Write(outChunk.AddConstant(funcName));
		outChunk.Write(argCount);
		return true;
	};

	auto parseGetUpValue = [&]() -> bool {
		const auto argOpt = lexer.next();
		outChunk.Write(static_cast<uint8_t>(OpCode::GetUpvalue));
		outChunk.Write(static_cast<uint8_t>(std::stoul(std::string(argOpt->lexeme))));
		return true;
	};

	auto parseSetUpValue = [&]() -> bool {
		const auto argOpt = lexer.next();
		outChunk.Write(static_cast<uint8_t>(OpCode::SetUpvalue));
		outChunk.Write(static_cast<uint8_t>(std::stoul(std::string(argOpt->lexeme))));
		return true;
	};
	auto parseLoadFun = [&]() -> bool {
		const auto funcNameOpt = lexer.next();
		if (!funcNameOpt)
		{
			return false;
		}

		outChunk.Write(static_cast<std::uint8_t>(OpCode::MakeClosure));
		fixups.push_back({ outChunk.Size(), String(funcNameOpt->lexeme) });
		outChunk.Write(0);
		outChunk.Write(0);

		return true;
	};

	while (const auto tokenOpt = lexer.next())
	{
		const auto& token = *tokenOpt;
		if (token.type != TokenType::Identifier)
		{
			std::cerr << "Unexpected token: " << token.lexeme << "\n";
			return false;
		}

		switch (HashedString(token.lexeme))
		{
			// clang-format off
          case "ADD"_hs:    	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Add)); break;
          case "SUB"_hs:    	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Sub)); break;
          case "MUL"_hs:    	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Mul)); break;
          case "DIV"_hs:    	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Div)); break;
		  case "INC"_hs:           outChunk.Write(static_cast<std::uint8_t>(OpCode::Inc)); break;
		  case "DEC"_hs:           outChunk.Write(static_cast<std::uint8_t>(OpCode::Dec)); break;
		  case "BIT_NOT"_hs:       outChunk.Write(static_cast<std::uint8_t>(OpCode::BitNot)); break;
          case "POP"_hs:    	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Pop)); break;
          case "RETURN"_hs: 	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Return)); break;

		  case "LESS"_hs:   	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Less)); break;
		  case "EQUAL"_hs:  	   outChunk.Write(static_cast<std::uint8_t>(OpCode::Equal)); break;

		  case "LABEL"_hs:		   if (!parseLabel()) return false; break;
		  case "JMP"_hs:		   if (!parseJump(OpCode::Jmp)) return false; break;
		  case "JMP_IF_FALSE"_hs:  if (!parseJump(OpCode::JmpIfFalse)) return false; break;

          case "CONST"_hs:  	   if (!parseConst()) return false; break;
          case "SET"_hs:    	   if (!parseSet()) return false; break;
          case "GET"_hs:    	   if (!parseGet()) return false; break;
          case "FUN"_hs:    	   if (!parseDef()) return false; break;

		  case "NEW"_hs:           if (!parseNew()) return false; break;
		  case "GET_PROPERTY"_hs:  if (!parseGetProperty()) return false; break;
		  case "SET_PROPERTY"_hs:  if (!parseSetProperty()) return false; break;
		  case "TYPE_OF"_hs:       outChunk.Write(static_cast<std::uint8_t>(OpCode::TypeOf)); break;
		  case "TYPE"_hs:          if (!parseType()) return false; break;

		  case "MAKE_ARRAY"_hs:    outChunk.Write(static_cast<std::uint8_t>(OpCode::MakeArray)); break;
		  case "INDEX_LOAD"_hs:    outChunk.Write(static_cast<std::uint8_t>(OpCode::IndexLoad)); break;
		  case "INDEX_STORE"_hs:   outChunk.Write(static_cast<std::uint8_t>(OpCode::IndexStore)); break;

		  case "BOX"_hs:           outChunk.Write(static_cast<uint8_t>(OpCode::Box)); break;
		  case "UNBOX"_hs:         outChunk.Write(static_cast<uint8_t>(OpCode::Unbox)); break;
		  case "STORE_BOX"_hs:     outChunk.Write(static_cast<uint8_t>(OpCode::StoreBox)); break;

		  case "GET_UPVALUE"_hs:   parseGetUpValue(); break;
		  case "SET_UPVALUE"_hs:   parseSetUpValue(); break;

		  case "MAKE_CLOSURE"_hs:  if (!parseMakeClosure()) return false; break;
		  case "LOAD_NATIVE"_hs:   if (!parseLoadNative()) return false; break;

		  case "CALL"_hs:   	   if (!parseCall())   return false; break;
          case "NATIVE"_hs: 	   if (!parseNative()) return false; break;
		  case "LOAD_FUN"_hs:      if (!parseLoadFun()) return false; break;
		  case "CALL_INDIRECT"_hs: if (!parseCallIndirect()) return false; break;
			// clang-format on
		default:
			std::cerr << "Unknown instruction: " << token.lexeme << "\n";
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

		outChunk.Patch(codeOffset, outChunk.AddConstant(labels[hash]));
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