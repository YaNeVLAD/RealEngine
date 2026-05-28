#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

namespace igni::dotnet
{

class TypeMapper
{
public:
	static re::String MapToCIL(const re::String& semTypeName);

	static re::String MapAstType(const ast::TypeNode* node);

	static re::String GetElemSuffix(const re::String& semTypeName);

	static std::string BuildParamSignature(const std::vector<ast::Parameter>& params, bool isVararg = false);
};

} // namespace igni::dotnet