#pragma once

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
	const char* file,
	const int line,
	std::format_string<Args...> message,
	Args&&... args)
{
	std::cerr << "========================================\n"
			  << "[RE_ASSERT FAILED]\n"
			  << "Expression: " << expr << "\n"
			  << "Location:   " << file << ":" << line << "\n"
			  << "Message:    " << std::format(message, std::forward<Args>(args)...) << "\n"
			  << "========================================\n";
}

#if !defined(NDEBUG)
#define RE_ASSERT(expr, ...)                                                                                         \
	do                                                                                                               \
	{                                                                                                                \
		if (!(expr))                                                                                                 \
		{                                                                                                            \
			re::detail::ReportAssertionFailure(#expr, re::detail::ExtractFileName(__FILE__), __LINE__, __VA_ARGS__); \
			RE_DEBUG_BREAK();                                                                                        \
		}                                                                                                            \
	} while (false)
#else
#define RE_ASSERT(expr, ...) \
	do                       \
	{                        \
		(void)(expr);        \
	} while (false)
#endif

} // namespace re::detail