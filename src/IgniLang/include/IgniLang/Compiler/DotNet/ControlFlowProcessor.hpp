#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Compiler/DotNet/fwd.hpp>

namespace igni::dotnet
{

class ControlFlowProcessor
{
protected:
	DotNetCodeGenerator& m_gen;

public:
	explicit ControlFlowProcessor(DotNetCodeGenerator& gen)
		: m_gen(gen)
	{
	}
	void ProcessForStmt(const ast::ForStmt* node) const;
	void ProcessIfStmt(const ast::IfStmt* node) const;
	void ProcessWhileStmt(const ast::WhileStmt* node) const;

private:
	void ProcessForEachLoop(const ast::ForStmt* node) const;
	void ProcessStandardForLoop(const ast::ForStmt* node) const;
};

} // namespace igni::dotnet