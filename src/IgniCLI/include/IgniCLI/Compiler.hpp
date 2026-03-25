#pragma once

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
		const auto linkedProgram = std::make_unique<ast::Program>();
		linkedProgram->packageName = "global";

		fsm::slr::parser parser(m_table, "<EPSILON>");
		AstConverter astConverter;

		std::list<std::string> sourceBuffers;

		// --- PHASE 1: FRONTEND ---
		for (const auto& path : filePaths)
		{
			std::cout << "[Info] Parsing " << path << "...\n";

			sourceBuffers.emplace_back(ReadFile(path));
			const std::string& source = sourceBuffers.back();

			auto tokens = CreateLexer(source).tokenize();
			auto cstRoot = CstBuilder(parser, "<EPSILON>").Build(tokens);

			if (!cstRoot)
			{
				throw std::runtime_error("Syntax error in file: " + path);
			}

			const auto astRoot = astConverter.Convert(cstRoot.get());
			if (!astRoot)
			{
				throw std::runtime_error("AST conversion failed for file: " + path);
			}

			// --- PHASE 2: AST LINKING ---
			for (auto& imp : astRoot->imports)
			{
				linkedProgram->imports.push_back(std::move(imp));
			}
			for (auto& stmt : astRoot->statements)
			{
				linkedProgram->statements.push_back(std::move(stmt));
			}
		}

		// --- PHASE 3: SEMANTIC ANALYSIS ---
		std::cout << "[Info] Running Semantic Analysis...\n";
		sem::SemanticAnalyzer semanticAnalyzer;
		semanticAnalyzer.Analyze(linkedProgram.get());
		auto globalNames = semanticAnalyzer.GetGlobalNames();
		auto& importAliases = semanticAnalyzer.GetImportAliases();

		// --- PHASE 4: CODE GENERATION ---
		std::cout << "[Info] Generating Bytecode...\n";
		compiler::TextCompiler textCompiler;
		std::string assembly = textCompiler.Compile(linkedProgram.get(), globalNames, importAliases);

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
		file.read(buffer.data(), fileSize);
		return buffer;
	}
};

} // namespace igni