#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Compiler/DotNet/fwd.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <vector>

namespace igni::dotnet::SignatureUtils
{

re::String BuildTypeSignature(const std::vector<std::shared_ptr<sem::SemanticType>>& types, size_t startIndex, bool isVararg);

re::String BuildTypeSignature(const std::vector<std::unique_ptr<ast::ParameterNode>>& params, size_t startIndex, bool isVararg);

re::String BuildCilSignature(const sem::FunctionType* funType, const re::String& ownerClass, const re::String& methodName, bool isInstance, bool skipFirstParam);

re::String GetDelegateName(const sem::FunctionType* funType);

} // namespace igni::dotnet::SignatureUtils