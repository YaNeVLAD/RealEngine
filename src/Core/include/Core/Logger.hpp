#pragma once

#include <Core/Export.hpp>

#include <format>
#include <iostream>
#include <string_view>

namespace re
{

enum class LogLevel
{
	Info,
	Warning,
	Error,
	Fatal,
};

struct LogCategory
{
	std::string_view name{};

	LogCategory() = default;

	explicit LogCategory(const std::string_view name)
		: name(name)
	{
	}

	explicit operator std::string_view() const
	{
		return name;
	}
};

class RE_CORE_API Logger
{
public:
	Logger() = delete;

	template <typename... Args>
	static constexpr void Log(LogLevel level, LogCategory category, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static constexpr void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Info(LogCategory category, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Info(std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Warn(LogCategory category, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Warn(std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Error(LogCategory category, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Error(std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Fatal(LogCategory category, std::format_string<Args...> fmt, Args&&... args);

	template <typename... Args>
	static void Fatal(std::format_string<Args...> fmt, Args&&... args);

private:
	static inline const char* LevelToString(LogLevel level);
};

#define RE_LOG_INFO(...) ::re::Logger::Info(__VA_ARGS__)
#define RE_LOG_WARN(...) ::re::Logger::Warn(__VA_ARGS__)
#define RE_LOG_ERROR(...) ::re::Logger::Error(__VA_ARGS__)
#define RE_LOG_FATAL(...) ::re::Logger::Fatal(__VA_ARGS__)

namespace literals
{

inline LogCategory operator""_logcat(const char* str, const std::size_t len)
{
	return LogCategory{ std::string_view{ str, len } };
}

} // namespace literals

} // namespace re

#include <Core/Logger.inl>