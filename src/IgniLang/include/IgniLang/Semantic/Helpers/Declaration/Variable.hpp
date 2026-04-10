#pragma once

#include <IgniLang/Export.hpp>

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/Context.hpp>

namespace igni::sem::Declaration
{

IGNI_API void Val(const re::String& name, const ast::Expr* initializer, const ast::TypeNode* explicitTypeNode, SemanticContext& m_context);

IGNI_API void Var(const re::String& name, const ast::Expr* initializer, const ast::TypeNode* explicitTypeNode, SemanticContext& m_context);

} // namespace igni::sem::Declaration