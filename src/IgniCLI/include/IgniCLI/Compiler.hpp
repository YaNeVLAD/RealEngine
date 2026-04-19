#pragma once

#include "IgniLang/Diagnostic/Diagnostic.hpp"
#include "IgniLang/Optimization/DeadCodeEliminator.hpp"

#include <IgniLang/AST/AstConverter.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/CST/CstBuilder.hpp>
#include <IgniLang/Compiler/TextCompiler.hpp>
#include <IgniLang/LexerFactory.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <fsm/cfg.hpp>
#include <fsm/slr/parser.hpp>
#include <fsm/slr/table_builder.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace igni
{

class Compiler
{
public:
	explicit Compiler(const std::string& grammarPath)
	{
		std::ifstream file(grammarPath);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open grammar file: " + grammarPath);
		}

		const auto grammar = fsm::cfg_load<re::String>(file);
		fsm::slr::table_builder builder(grammar);
		m_table = builder
					  .with_epsilon("<EPSILON>")
					  .with_end_marker("<EOF>")
					  .build(fsm::slr::collision_policy::prefer_shift);
	}

	[[nodiscard]] std::string CompileFiles(const std::vector<std::string>& filePaths) const
	{
		fsm::slr::parser parser(m_table, "<EPSILON>");
		AstConverter astConverter;

		std::list<std::string> sourceBuffers;

		std::vector<std::unique_ptr<ast::Program>> parsedPrograms;

		DiagnosticEngine diagnostics;

		// --- PHASE 1: FRONTEND ---
		for (const auto& path : filePaths)
		{
			std::cout << "[Info] Parsing " << path << "...\n";

			sourceBuffers.emplace_back(ReadFile(path));
			const std::string& source = sourceBuffers.back();

			auto tokens = CreateLexer(source).tokenize();

			std::vector<fsm::token<TokenType>> validTokens;
			validTokens.reserve(tokens.size());

			for (const auto token : tokens)
			{
				if (token.type == TokenType::Error)
				{
					diagnostics.ReportError(
						"Unexpected character '" + std::string(token.lexeme) + "'",
						token.line,
						token.column,
						token.offset,
						token.length);
				}
				else
				{
					validTokens.emplace_back(token);
				}
			}

			std::shared_ptr<CstNode> cstRoot = nullptr;
			try
			{
				cstRoot = CstBuilder(parser, "<EPSILON>").Build(validTokens);
			}
			catch (const std::exception& e)
			{
				diagnostics.ReportError("Syntax Error in " + path + ": " + e.what(), 0, 0, 0, 0);
				continue;
			}

			if (!cstRoot)
			{
				diagnostics.ReportError("Failed to build syntax tree for file: " + path, 0, 0, 0, 0);
				continue;
			}

			std::unique_ptr<ast::Program> astRoot = nullptr;
			try
			{
				astRoot = astConverter.Convert(cstRoot.get());
			}
			catch (const std::exception& e)
			{
				diagnostics.ReportError("AST Conversion Error in " + path + ": " + e.what(), 0, 0, 0, 0);
				continue;
			}

			if (!astRoot)
			{
				diagnostics.ReportError("AST conversion returned null for file: " + path, 0, 0, 0, 0);
				continue;
			}

			parsedPrograms.push_back(std::move(astRoot));
		}

		if (diagnostics.HasErrors())
		{
			diagnostics.PrintToConsole();
			return {};
		}

		sem::g_Diagnostics = &diagnostics;

		// --- PHASE 2: SEMANTIC ANALYSIS ---
		std::cout << "[Info] Running Semantic Analysis...\n";
		sem::SemanticAnalyzer semanticAnalyzer;

		try
		{
			semanticAnalyzer.Analyze(parsedPrograms);
		}
		catch (const std::exception& e)
		{
			diagnostics.ReportError(std::string("Fatal Semantic Error: ") + e.what(), 0, 0, 0, 0);
		}

		sem::g_Diagnostics = nullptr;

		if (diagnostics.HasErrors())
		{
			diagnostics.PrintToConsole();
			return {};
		}

		const auto& globalNames = semanticAnalyzer.GetGlobalNames();
		const auto& importAliases = semanticAnalyzer.GetImportAliases();
		const auto& externals = semanticAnalyzer.GetExternalFunctions();

		// --- PHASE 3: AST LINKING ---
		const auto linkedProgram = std::make_unique<ast::Program>();
		linkedProgram->packageName = "global";

		for (const auto& prog : parsedPrograms)
		{
			for (auto& imp : prog->imports)
			{
				linkedProgram->imports.push_back(std::move(imp));
			}
			for (auto& stmt : prog->statements)
			{
				linkedProgram->statements.push_back(std::move(stmt));
			}
		}

		// --- PHASE 4: OPTIMIZATION ---
		opt::DeadCodeEliminator dce;
		dce.Eliminate(linkedProgram.get());

		// --- PHASE 5: CODE GENERATION ---
		std::cout << "[Info] Generating Bytecode...\n";
		compiler::TextCompiler textCompiler;
		std::string assembly = textCompiler.Compile(linkedProgram.get(), globalNames, importAliases, externals, semanticAnalyzer);

		return assembly;
	}

private:
	fsm::slr::table<re::String> m_table;

	static std::string ReadFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::ate);
		if (!file.is_open())
		{
			throw std::runtime_error("Cannot open source file: " + path);
		}

		const std::size_t fileSize = file.tellg();
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

		return buffer;
	}
};

} // namespace igni