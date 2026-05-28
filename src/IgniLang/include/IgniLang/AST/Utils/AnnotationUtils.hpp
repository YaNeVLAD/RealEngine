#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

#include <optional>

namespace igni::ast::AnnotationUtils
{

std::optional<re::String> GetAnnotationStringArg(const Annotation& anno);

std::optional<re::String> GetAnnotationArg(const std::vector<Annotation>& annotations, const re::String& targetName);

} // namespace igni::ast