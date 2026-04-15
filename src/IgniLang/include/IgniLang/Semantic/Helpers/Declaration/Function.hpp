#pragma once

#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <memory>

namespace igni::sem::Declaration
{

re::String InjectThisKeyword(ast::FunDecl* decl, const re::String& className);

std::shared_ptr<FunctionType> Function(ast::FunDecl* decl, SemanticContext& ctx, const re::String& moduleName);

std::shared_ptr<FunctionType> Method(ast::FunDecl* decl, const re::String& originalName, const std::shared_ptr<ClassType>& classType, SemanticContext& ctx);

std::shared_ptr<FunctionType> Constructor(ast::ConstructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx);

std::shared_ptr<FunctionType> Destructor(ast::DestructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx);

} // namespace igni::sem::Declaration