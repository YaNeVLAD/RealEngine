#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Compiler/DotNet/fwd.hpp>

#include <vector>

namespace igni::dotnet
{

class GlobalProcessor
{
protected:
	DotNetCodeGenerator& m_gen;

public:
	explicit GlobalProcessor(DotNetCodeGenerator& gen)
		: m_gen(gen)
	{
	}

	void ProcessAssemblies(const ast::Program* program) const;
	void ProcessGlobalFields(const ast::Program* program) const;
	void ProcessGlobalConstructor(const ast::Program* program) const;

private:
	void ScanAnnotationsForAssembly(const std::vector<std::unique_ptr<ast::AnnotationNode>>& annotations) const;
	void ExtractAssemblies(const re::String& signature) const;
};

} // namespace igni::dotnet