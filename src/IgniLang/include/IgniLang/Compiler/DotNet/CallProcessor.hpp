#pragma once

#include <Core/flat_map.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Compiler/DotNet/GlobalProcessor.hpp>
#include <IgniLang/Compiler/DotNet/LambdaHelper.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

namespace igni::dotnet
{

class CallProcessor
{
protected:
	DotNetCodeGenerator& m_gen;

public:
	explicit CallProcessor(DotNetCodeGenerator& gen)
		: m_gen(gen)
	{
	}
	void ProcessCallExpr(const ast::CallExpr* node) const;

private:
	void ProcessArguments(const ast::CallExpr* node, const CallInfo& callInfo, bool ignoreVararg = false) const;
	void ProcessVarargArguments(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void PackVarargsLoop(const ast::CallExpr* node, std::size_t normalCount, std::size_t varargCount, const re::String& elemType) const;
	bool TryProcessIndirectDelegate(const ast::CallExpr* node) const;
	bool TryProcessInlineOpcode(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessLdelem(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessStelem(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessIndirectCall(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessNativeCall(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessConstructorCall(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessVirtualCall(const ast::CallExpr* node, const CallInfo& callInfo) const;
	void ProcessGlobalOrHoistedCall(const ast::CallExpr* node, const CallInfo& callInfo) const;
};

} // namespace igni::dotnet