#include <IgniLang/Semantic/Helpers/Declaration/Variable.hpp>

#include <IgniLang/Semantic/Helpers/TypeResolver.hpp>

namespace
{

void DeclareVariable(
	const re::String& name,
	const igni::ast::Expr* initializer,
	const igni::ast::TypeNode* explicitTypeNode,
	const bool isReadOnly,
	igni::sem::SemanticContext& m_context)
{
	auto varType = m_context.evaluateFunctionCallback(initializer);

	if (explicitTypeNode)
	{
		const auto declaredType = igni::sem::TypeResolver::Resolve(explicitTypeNode, m_context);
		if (initializer)
		{
			igni::sem::TypeResolver::ExpectAssignable(varType.get(), declaredType.get(), "variable initialization", initializer);
		}
		varType = declaredType;
	}

	if (varType == m_context.tUnit && !explicitTypeNode)
	{
		IGNI_SEM_ERR(initializer, "Semantic Error: Cannot infer type for variable '" + name + "'");
	}

	m_context.env.Define(name, varType, isReadOnly);
}

} // namespace

namespace igni::sem::Declaration
{

void Val(const re::String& name, const ast::Expr* initializer, const ast::TypeNode* explicitTypeNode, SemanticContext& m_context)
{
	DeclareVariable(name, initializer, explicitTypeNode, true, m_context);
}

void Var(const re::String& name, const ast::Expr* initializer, const ast::TypeNode* explicitTypeNode, SemanticContext& m_context)
{
	DeclareVariable(name, initializer, explicitTypeNode, false, m_context);
}

} // namespace igni::sem::Declaration