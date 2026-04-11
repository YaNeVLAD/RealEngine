#pragma once

#include <Core/String.hpp>

#include <stdexcept>

namespace igni::sem
{

[[noreturn]] inline void SemanticError(const ast::Node* node, const re::String& message)
{
	const re::String location = "[Line: " + std::to_string(node->token.line) + ", Col: " + std::to_string(node->token.column) + "]";

	const re::String fullMessage = location + " Semantic Error: " + message + "\n    Near: '" + std::string(node->token.lexeme) + "'";

	throw std::runtime_error(fullMessage.ToString());
}

[[noreturn]] inline void InternalError(const ast::Node* node, const re::String& message)
{
	const re::String location = "[Line: " + std::to_string(node->token.line) + ", Col: " + std::to_string(node->token.column) + "]";

	const re::String fullMessage = location + " Compiler Internal Error: " + message + "\n    Near: '" + std::string(node->token.lexeme) + "'";

	throw std::runtime_error(fullMessage.ToString());
}

#define _IGNI_EXPAND(x) x

#define _GET_SEM_MACRO(_1, _2, NAME, ...) NAME

#define _IGNI_SEM_ERR_1(msg) igni::sem::SemanticError(node, msg)
#define _IGNI_SEM_ERR_2(node_ptr, msg) igni::sem::SemanticError(node_ptr, msg)

#define IGNI_SEM_ERR(...) \
	_IGNI_EXPAND(_GET_SEM_MACRO(__VA_ARGS__, _IGNI_SEM_ERR_2, _IGNI_SEM_ERR_1)(__VA_ARGS__))

#define _IGNI_INT_ERR_1(msg) igni::sem::InternalError(node, msg)
#define _IGNI_INT_ERR_2(node_ptr, msg) igni::sem::InternalError(node_ptr, msg)

#define IGNI_INTERNAL_ERR(...) \
	_IGNI_EXPAND(_GET_SEM_MACRO(__VA_ARGS__, _IGNI_INT_ERR_2, _IGNI_INT_ERR_1)(__VA_ARGS__))

} // namespace igni::sem