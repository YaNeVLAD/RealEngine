#include <IgniLang/AST/Utils/AnnotationUtils.hpp>

namespace igni::ast::AnnotationUtils
{

std::optional<re::String> GetAnnotationArg(const std::vector<std::unique_ptr<AnnotationNode>>& annotations, const re::String& targetName)
{
	for (const auto& anno : annotations)
	{
		if (anno && anno->name == targetName)
		{
			return GetAnnotationStringArg(anno.get());
		}
	}

	return std::nullopt;
}

std::optional<re::String> GetAnnotationArg(const std::vector<const AnnotationNode*>& annotations, const re::String& targetName)
{
	for (const auto& anno : annotations)
	{
		if (anno && anno->name == targetName)
		{
			return GetAnnotationStringArg(anno);
		}
	}

	return std::nullopt;
}

std::optional<re::String> GetAnnotationStringArg(const AnnotationNode* anno)
{
	if (!anno)
	{
		return std::nullopt;
	}

	if (anno->argument)
	{
		if (const auto lit = dynamic_cast<const LiteralExpr*>(anno->argument.get()))
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