#pragma once

#include <Core/FileCache.hpp>
#include <GeneratedSemantics.hpp>
#include <IgniLang/AST/AstConverter.hpp>
#include <IgniLang/BuildType.hpp>
#include <IgniLang/CST/CstBuilder.hpp>
#include <IgniLang/Compiler/IBackend.hpp>
#include <IgniLang/Diagnostic/Diagnostic.hpp>
#include <IgniLang/Internal/GrammarCachePolicy.hpp>
#include <IgniLang/LexerFactory.hpp>
#include <IgniLang/Optimization/DeadCodeEliminator.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <fsm/lr/parser.hpp>
#include <fsm/lr/table_io.hpp>

#include <filesystem>
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
		const std::string cachePath = grammarPath + ".bin";

		m_table = re::FileCache::Execute(grammarPath, cachePath, detail::GrammarCachePolicy{});
	}

	CompilationResult Compile(
		const std::vector<re::String>& filePaths,
		BuildTarget target,
		BuildType buildType,
		bool disableDCE,
		IBackend& backend) const
	{
		CompilationResult result;
		DiagnosticEngine diagnostics;

		std::list<std::string> sourceBuffers;

		// --- PHASE 1: FRONTEND ---
		const auto parsedPrograms = RunFrontend(filePaths, diagnostics, sourceBuffers);
		if (diagnostics.HasErrors())
		{
			diagnostics.PrintToConsole();
			return result;
		}

		// --- PHASE 2: SEMANTICS ---
		std::cout << "[Info] Running Semantic Analysis for target: " << static_cast<int>(target) << "...\n";
		auto config = sem::generated::GetTargetConfig(target);
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
		dce.Eliminate(linkedProgram.get(), semanticAnalyzer->GetBindings(), buildType == BuildType::DynamicLibrary, disableDCE);

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
	fsm::lr::table<re::String> m_table;

	std::vector<std::unique_ptr<ast::Program>> RunFrontend(
		const std::vector<re::String>& filePaths,
		DiagnosticEngine& diagnostics,
		std::list<std::string>& sourceBuffers) const
	{
		fsm::lr::parser parser(m_table, "<EPSILON>");
		AstConverter astConverter;
		std::vector<std::unique_ptr<ast::Program>> programs;

		for (const auto& path : filePaths)
		{
			std::cout << "[Info] Parsing " << path << "...\n";
			sourceBuffers.emplace_back(ReadFile(path));
			const std::string& source = sourceBuffers.back();
			auto tokens = CreateLexer(source).tokenize();
			if (!tokens)
			{
				std::cerr << "Failed to tokenize file " << path;
				continue;
			}

			std::vector<fsm::token<TokenType>> validTokens;
			for (const auto& token : *tokens)
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

} // namespace igni::compiler