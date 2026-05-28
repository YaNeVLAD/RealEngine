#include <IgniLang/AST/Utils/AnnotationUtils.hpp>

namespace igni::ast::AnnotationUtils
{

std::optional<re::String> GetAnnotationArg(const std::vector<Annotation>& annotations, const re::String& targetName)
{
	for (const auto& anno : annotations)
	{
		if (anno.name == targetName)
		{
			return GetAnnotationStringArg(anno);
		}
	}

	return std::nullopt;
}

std::optional<re::String> GetAnnotationStringArg(const Annotation& anno)
{
	if (anno.argument)
	{
		if (const auto lit = dynamic_cast<const LiteralExpr*>(anno.argument.get()))
		{
			if (lit->token.type == TokenType::StringConst)
			{
				return lit->token.lexeme.substr(1, lit->token.lexeme.length() - 2);
			}
		}
	}

	return std::nullopt;
}

} // namespace igni::ast::AnnotationUtils