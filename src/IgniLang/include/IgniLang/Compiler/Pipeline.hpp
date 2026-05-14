#pragma once

#include <GeneratedSemantics.hpp>
#include <IgniLang/AST/AstConverter.hpp>
#include <IgniLang/CST/CstBuilder.hpp>
#include <IgniLang/Compiler/IBackend.hpp>
#include <IgniLang/Diagnostic/Diagnostic.hpp>
#include <IgniLang/LexerFactory.hpp>
#include <IgniLang/Optimization/DeadCodeEliminator.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <fsm/cfg.hpp>
#include <fsm/slr/parser.hpp>
#include <fsm/slr/table_builder.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace igni::compiler
{

struct CompilationResult
{
	bool success = false;
	std::string generatedCode;
	std::shared_ptr<sem::SemanticAnalyzer> semantics;
};

class Pipeline
{
public:
	explicit Pipeline(const std::string& grammarPath)
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

	CompilationResult Compile(
		const std::vector<std::string>& filePaths,
		const std::string& targetId,
		compiler::IBackend& backend) const
	{
		CompilationResult result;
		DiagnosticEngine diagnostics;

		// --- PHASE 1: FRONTEND ---
		auto parsedPrograms = RunFrontend(filePaths, diagnostics);
		if (diagnostics.HasErrors())
		{
			diagnostics.PrintToConsole();
			return result;
		}

		// --- PHASE 2: SEMANTICS ---
		std::cout << "[Info] Running Semantic Analysis for target: " << targetId << "...\n";
		auto config = sem::generated::GetTargetConfig(targetId);
		auto semanticAnalyzer = std::make_shared<sem::SemanticAnalyzer>(config);

		sem::g_Diagnostics = &diagnostics;

		try
		{
			semanticAnalyzer->Analyze(parsedPrograms);
		}
		catch (const std::exception& e)
		{
			diagnostics.ReportError(std::string("Fatal Semantic Error: ") + e.what(), 0, 0, 0, 0);
		}

		sem::g_Diagnostics = nullptr;

		if (diagnostics.HasErrors())
		{
			diagnostics.PrintToConsole();
			return result;
		}

		// --- PHASE 3: AST LINKING ---
		auto linkedProgram = LinkAsts(parsedPrograms);

		// --- PHASE 4: OPTIMIZATION ---
		opt::DeadCodeEliminator dce;
		dce.Eliminate(linkedProgram.get(), semanticAnalyzer->GetBindings());

		// --- PHASE 5: CODE GENERATION ---
		std::cout << "[Info] Generating Code using injected Backend...\n";
		result.generatedCode = backend.Generate(
			linkedProgram.get(),
			semanticAnalyzer->GetGlobalNames(),
			semanticAnalyzer->GetImportAliases(),
			semanticAnalyzer->GetExternalFunctions(),
			*semanticAnalyzer);

		result.success = true;
		result.semantics = semanticAnalyzer;

		return result;
	}

private:
	fsm::slr::table<re::String> m_table;

	std::vector<std::unique_ptr<ast::Program>> RunFrontend(const std::vector<std::string>& filePaths, DiagnosticEngine& diagnostics) const
	{
		fsm::slr::parser parser(m_table, "<EPSILON>");
		AstConverter astConverter;
		std::vector<std::unique_ptr<ast::Program>> programs;

		for (const auto& path : filePaths)
		{
			std::cout << "[Info] Parsing " << path << "...\n";
			std::string source = ReadFile(path);
			auto tokens = CreateLexer(source).tokenize();

			std::vector<fsm::token<TokenType>> validTokens;
			for (const auto& token : tokens)
			{
				if (token.type == TokenType::Error)
				{
					diagnostics.ReportError("Unexpected character", token.line, token.column, token.offset, token.length);
				}
				else
				{
					validTokens.emplace_back(token);
				}
			}

			try
			{
				auto cstRoot = CstBuilder(parser, "<EPSILON>").Build(validTokens);
				if (cstRoot)
				{
					if (auto astRoot = astConverter.Convert(cstRoot.get()))
					{
						programs.push_back(std::move(astRoot));
					}
				}
			}
			catch (const std::exception& e)
			{
				diagnostics.ReportError("Frontend Error in " + path + ": " + e.what(), 0, 0, 0, 0);
			}
		}

		return programs;
	}

	static std::unique_ptr<ast::Program> LinkAsts(const std::vector<std::unique_ptr<ast::Program>>& programs)
	{
		auto linked = std::make_unique<ast::Program>();
		linked->packageName = "global";
		for (const auto& prog : programs)
		{
			for (auto& imp : prog->imports)
			{
				linked->imports.push_back(std::move(imp));
			}
			for (auto& stmt : prog->statements)
			{
				linked->statements.push_back(std::move(stmt));
			}
		}

		return linked;
	}

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