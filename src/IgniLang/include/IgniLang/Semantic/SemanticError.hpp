#pragma once

#include <Core/Assert.hpp>
#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Diagnostic/Diagnostic.hpp>

#include <stdexcept>

namespace igni::sem
{

struct SemanticBailoutException final : std::exception
{
	const char* what() const noexcept override { return "Semantic bailout"; }
};

inline thread_local DiagnosticEngine* g_Diagnostics = nullptr;

[[noreturn]] inline void SemanticError(const ast::Node* node, const re::String& message)
{
	if (g_Diagnostics)
	{
		const std::size_t line = node ? node->token.line : 0;
		const std::size_t col = node ? node->token.column : 0;
		const std::size_t offset = node ? node->token.offset : 0;
		const std::size_t length = (node && node->token.length > 0) ? node->token.length : 1;

		g_Diagnostics->ReportError(message.ToString(), line, col, offset, length);
	}
	else
	{
		std::cerr << "Semantic Error: " << message << std::endl;
	}

	throw SemanticBailoutException();
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