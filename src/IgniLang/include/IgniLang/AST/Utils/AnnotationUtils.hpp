#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace igni::ast::AnnotationUtils
{

std::optional<re::String> GetAnnotationStringArg(const AnnotationNode* anno);

std::optional<re::String> GetAnnotationArg(const std::vector<std::unique_ptr<AnnotationNode>>& annotations, const re::String& targetName);

std::optional<re::String> GetAnnotationArg(const std::vector<const AnnotationNode*>& annotations, const re::String& targetName);

} // namespace igni::ast::AnnotationUtils