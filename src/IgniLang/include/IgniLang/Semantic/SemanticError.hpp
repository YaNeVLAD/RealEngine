#pragma once

#include <Core/String.hpp>

#include <stdexcept>

namespace igni::sem
{

template <typename TNode>
[[noreturn]] void SemanticError(const TNode* node, const re::String& message)
{
	const re::String location = "[Line: " + std::to_string(node->token.line) + ", Col: " + std::to_string(node->token.column) + "]";

	const re::String fullMessage = location + " Semantic Error: " + message + "\n    Near: '" + std::string(node->token.lexeme) + "'";

	throw std::runtime_error(fullMessage.ToString());
}

template <typename TNode>
[[noreturn]] void InternalError(const TNode* node, const re::String& message)
{
	const re::String location = "[Line: " + std::to_string(node->token.line) + ", Col: " + std::to_string(node->token.column) + "]";

	const re::String fullMessage = location + " Compiler Internal Error: " + message + "\n    Near: '" + std::string(node->token.lexeme) + "'";

	throw std::runtime_error(fullMessage.ToString());
}

} // namespace igni::sem