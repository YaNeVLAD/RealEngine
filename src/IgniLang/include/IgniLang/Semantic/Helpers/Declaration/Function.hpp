#pragma once

#include <IgniLang/Semantic/Context.hpp>
#include <IgniLang/Semantic/SemanticType.hpp>

#include <memory>

namespace igni::sem::Declaration
{

std::shared_ptr<FunctionType> Function(const ast::FunDecl* decl, SemanticContext& ctx, const re::String& moduleName);

std::shared_ptr<FunctionType> Method(const ast::FunDecl* decl, const std::shared_ptr<ClassType>& classType, SemanticContext& ctx);

std::shared_ptr<FunctionType> Constructor(const ast::ConstructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx);

std::shared_ptr<FunctionType> Destructor(const ast::DestructorDecl* decl, const std::shared_ptr<ClassType>& classType, bool isClassExternal, SemanticContext& ctx);

std::shared_ptr<GenericFunctionTemplate> GenericFunction(const ast::FunDecl* decl, const re::String& moduleName, const std::shared_ptr<ClassType>& parentClass = nullptr);

} // namespace igni::sem::Declaration