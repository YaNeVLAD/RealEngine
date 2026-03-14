#pragma once

#include <Core/Logger.hpp>

#include <format>
#include <iostream>
#include <source_location>

#if defined(_MSC_VER)

#define RE_DEBUG_BREAK() __debugbreak()

#elif defined(__GNUC__) || defined(__clang__)

#define RE_DEBUG_BREAK() __builtin_trap()

#else

#define RE_DEBUG_BREAK() std::abort()

#endif

namespace re::detail
{

constexpr const char* ExtractFileName(const char* path)
{
	const char* file = path;
	while (*path)
	{
		if (*path == '/' || *path == '\\')
		{
			file = path + 1;
		}
		path++;
	}
	return file;
}

template <typename... Args>
constexpr void ReportAssertionFailure(
	const char* expr,
	const std::source_location location,
	std::format_string<Args...> message,
	Args&&... args)
{
	std::string userMsg = std::format(message, std::forward<Args>(args)...);

	std::string assertMsg = std::format(
		"========================================\n"
		"[RE_ASSERT FAILED]\n"
		"Expression: {}\n"
		"Location:   {}:{}\n"
		"Function:   {}\n"
		"Message:    {}\n"
		"========================================\n",
		expr,
		ExtractFileName(location.file_name()),
		location.line(),
		location.function_name(),
		userMsg);

	Logger::Fatal("{}", assertMsg);
}

#if defined(RE_DEBUG)
#define RE_ASSERT(expr, ...)                                                                         \
	do                                                                                               \
	{                                                                                                \
		if (!(expr))                                                                                 \
		{                                                                                            \
			re::detail::ReportAssertionFailure(#expr, std::source_location::current(), __VA_ARGS__); \
			RE_DEBUG_BREAK();                                                                        \
		}                                                                                            \
	} while (false)
#else
#define RE_ASSERT(expr, ...) \
	do                       \
	{                        \
		(void)sizeof(expr);  \
	} while (false)
#endif

} // namespace re::detail