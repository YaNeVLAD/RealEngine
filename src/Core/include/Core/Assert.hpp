#pragma once

#include <Core/Logger.hpp>

#include <format>
#include <source_location>

#if defined(_MSC_VER)

#define RE_DEBUG_BREAK() __debugbreak()
#define RE_ASSUME(cond) __assume(cond)

#elif defined(__GNUC__) || defined(__clang__)

#define RE_DEBUG_BREAK() __builtin_trap()
#define RE_ASSUME(cond) ((cond) ? static_cast<void>(0) : __builtin_unreachable())

#else

#define RE_DEBUG_BREAK() std::abort()
#define RE_ASSUME(cond) static_cast<void>(0)

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
		"\n========================================\n"
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
		RE_ASSUME(expr);                                                                             \
	} while (false)

#else

#define RE_ASSERT(expr, ...) RE_ASSUME(expr)
// #define RE_ASSUME(cond) (void)0

#endif

} // namespace re::detail