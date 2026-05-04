#include "TestUtils.hpp"

#include <fstream>
#include <sstream>

using namespace igni;

void* FrontendTestFixture::s_tablePtr = nullptr;
fsm::slr::parser<re::String>* FrontendTestFixture::s_parser = nullptr;

struct LexerTestCase
{
	std::string name;
	std::string source;
	std::vector<TokenType> expectedTokens;
};

class LexerTests : public ::testing::TestWithParam<LexerTestCase>
{
};

TEST_P(LexerTests, ProducesCorrectTokens)
{
	const auto& [name, source, expectedTokens] = GetParam();
	auto tokens = Tokenize(source);

	if (!tokens.empty() && tokens.back().type == TokenType::EndOfFile)
	{
		tokens.pop_back();
	}

	ASSERT_EQ(tokens.size(), expectedTokens.size()) << "Token count mismatch in test: " << name;

	for (size_t i = 0; i < tokens.size(); ++i)
	{
		EXPECT_EQ(tokens[i].type, expectedTokens[i])
			<< "Token mismatch at index " << i << " in test: " << name;
	}
}

INSTANTIATE_TEST_SUITE_P(
	IgniLexer,
	LexerTests,
	::testing::Values(
		LexerTestCase{
			"KeywordsAndIdentifiers",
			"val x = true",
			{ TokenType::KwVal, TokenType::Identifier, TokenType::Assign, TokenType::KwTrue } },
		LexerTestCase{
			"NumericLiterals",
			"42 3.14",
			{ TokenType::IntConst, TokenType::FloatConst } },
		LexerTestCase{
			"StringsAndSymbols",
			"\"hello\" :: ->",
			{ TokenType::StringConst, TokenType::DoubleColon, TokenType::Arrow } },
		LexerTestCase{
			"IgnoresCommentsAndWhitespace",
			"val a // comment \n = 1",
			{ TokenType::KwVal, TokenType::Identifier, TokenType::Assign, TokenType::IntConst } },
		LexerTestCase{
			"ErrorOnInvalidChar",
			"val $invalid = 1",
			{ TokenType::KwVal, TokenType::Error },
		}),
	[](const ::testing::TestParamInfo<LexerTestCase>& info) {
		return info.param.name;
	});

class ValidSyntaxTests : public FrontendTestFixture
	, public ::testing::WithParamInterface<std::pair<std::string, std::string>>
{
};

TEST_P(ValidSyntaxTests, ParsesSuccessfully)
{
	const std::string& source = GetParam().second;
	const auto ast = Parse(source);

	EXPECT_NE(ast, nullptr) << "Failed to parse valid syntax in test: " << GetParam().first;
}

INSTANTIATE_TEST_SUITE_P(
	IgniSyntax,
	ValidSyntaxTests,
	::testing::Values(
		std::make_pair("EmptyProgram", ""),
		std::make_pair("VariableDeclarations", "val x: Int = 5; var y = x;"),
		std::make_pair("Functions", "fun add(a: Int, b: Int): Int { return a + b; }"),
		std::make_pair("ExpressionBody", "fun mul(a: Int, b: Int) = a * b;"),
		std::make_pair("Generics", "class <T> Box(val item: T) { fun get(): T = this.item; }"),
		std::make_pair("AsyncAwait", "suspend fun fetch() { val data = await getNetwork(); launch process(data); }"),
		std::make_pair("ControlFlow", "fun loop() { for(i in 0..10) { if (i == 5) { return; } } }")),
	[](const ::testing::TestParamInfo<std::pair<std::string, std::string>>& info) { return info.param.first; });

class InvalidSyntaxTests : public FrontendTestFixture
	, public ::testing::WithParamInterface<std::pair<std::string, std::string>>
{
};

TEST_P(InvalidSyntaxTests, FailsGracefully)
{
	const std::string& source = GetParam().second;
	const auto ast = Parse(source);

	EXPECT_EQ(ast, nullptr) << "Invalid syntax surprisingly parsed successfully in test: " << GetParam().first;
}

INSTANTIATE_TEST_SUITE_P(
	IgniSyntaxError,
	InvalidSyntaxTests,
	::testing::Values(
		std::make_pair("MissingSemicolon", "val x = 5 val y = 6"),
		std::make_pair("MismatchedBrackets", "fun unclosed() { val a = 1; "),
		std::make_pair("BadFunctionDecl", "fun (a: Int) {}"),
		std::make_pair("InvalidExpression", "val x = + * 5;")),
	[](const ::testing::TestParamInfo<std::pair<std::string, std::string>>& info) { return info.param.first; });