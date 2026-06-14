#pragma once

#include "IgniLang/Internal/GrammarCachePolicy.hpp"

#include <gtest/gtest.h>

#include <Core/FileCache.hpp>
#include <IgniLang/AST/AstConverter.hpp>
#include <IgniLang/CST/CstBuilder.hpp>
#include <IgniLang/LexerFactory.hpp>

#include <fsm/lr/parser.hpp>

#include <filesystem>

inline std::vector<fsm::token<igni::TokenType>> Tokenize(const std::string& source)
{
	using namespace igni;

	auto lexer = CreateLexer(source);
	std::vector<fsm::token<TokenType>> tokens;
	while (true)
	{
		auto token = lexer.next();
		if (!token)
		{
			break;
		}
		if (!*token)
		{
			break;
		}

		tokens.push_back(**token);
		if ((*token)->type == TokenType::EndOfFile || (*token)->type == TokenType::Error)
		{
			break;
		}
	}
	return tokens;
}

inline std::string ExtractExpectedOutput(const std::filesystem::path& filePath)
{
	std::ifstream file(filePath);
	std::string line;
	std::stringstream expected;
	bool inExpectBlock = false;

	while (std::getline(file, line))
	{
		if (line.find("// EXPECT_OUTPUT:") != std::string::npos)
		{
			inExpectBlock = true;
			continue;
		}
		if (line.find("// END_EXPECT") != std::string::npos)
		{
			inExpectBlock = false;
			break;
		}
		if (inExpectBlock)
		{
			if (line.size() >= 3 && line.substr(0, 3) == "// ")
			{
				expected << line.substr(3) << "\n";
			}
			else if (line.size() >= 2 && line.substr(0, 2) == "//")
			{
				expected << line.substr(2) << "\n";
			}
		}
	}
	return expected.str();
}

inline std::vector<std::filesystem::path> GetTestScripts()
{
	namespace fs = std::filesystem;

	std::vector<fs::path> scripts;
	if (fs::exists("tests/scripts"))
	{
		for (const auto& entry : fs::directory_iterator("tests/scripts"))
		{
			if (entry.path().extension() == ".igni" && entry.path().string().find("_dotnet") == std::string::npos)
			{
				scripts.emplace_back(entry.path());
			}
		}
	}
	return scripts;
}

class FrontendTestFixture : public ::testing::Test
{
protected:
	static void* s_tablePtr;
	static fsm::lr::parser<re::String>* s_parser;

	static void SetUpTestSuite()
	{
		using namespace std::literals;

		const auto grammarPath = "assets/igni_grammar.txt"s;
		const std::string cachePath = grammarPath + ".bin";

		if (const std::ifstream file(grammarPath); !file.is_open())
		{
			throw std::runtime_error("Failed to open grammar file: assets/igni_grammar.txt");
		}

		auto table = re::FileCache::Execute(grammarPath, cachePath, igni::detail::GrammarCachePolicy{});

		auto* tablePtr = new auto(std::move(table));
		s_tablePtr = tablePtr;

		s_parser = new fsm::lr::parser(*tablePtr, "<EPSILON>");

		std::cout << "[Test Setup] SLR Parser table generated successfully.\n";
	}

	static void TearDownTestSuite()
	{
		delete s_parser;
		s_parser = nullptr;
	}

	std::unique_ptr<igni::ast::Program> Parse(const std::string& source)
	{
		using namespace igni;

		const auto tokens = Tokenize(source);

		if (!tokens.empty() && tokens.back().type == TokenType::Error)
		{
			return nullptr;
		}

		CstBuilder builder(*s_parser, "<EPSILON>");
		const auto cst = builder.Build(tokens);

		if (!cst)
		{
			return nullptr;
		}

		AstConverter converter;
		return converter.Convert(cst.get());
	}
};