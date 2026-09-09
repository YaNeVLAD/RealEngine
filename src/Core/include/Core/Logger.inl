#include <Core/Logger.hpp>

namespace re
{

template <typename... Args>
constexpr void Logger::Log(const LogLevel level, const LogCategory category, std::format_string<Args...> fmt, Args&&... args)
{
	std::string levelStr = LevelToString(level);
	std::string message = std::format(fmt, std::forward<Args>(args)...);

	const auto categoryName = static_cast<std::string_view>(category);
	std::string categoryPrefix = categoryName.empty() ? std::string{} : std::format("[{}]", categoryName);
	if (level == LogLevel::Error || level == LogLevel::Fatal)
	{
		std::cerr << std::format("[{}]{} {}\n", levelStr, categoryPrefix, message);
	}
	else
	{
		std::cout << std::format("[{}]{} {}\n", levelStr, categoryPrefix, message);
	}
}

template <typename... Args>
constexpr void Logger::Log(const LogLevel level, std::format_string<Args...> fmt, Args&&... args)
{
	Log(level, LogCategory{}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Info(LogCategory category, std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Info, category, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Info(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Info, LogCategory{}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Warn(LogCategory category, std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Warning, category, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Warn(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Warning, LogCategory{}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Error(LogCategory category, std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Error, category, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Error(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Error, LogCategory{}, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Fatal(LogCategory category, std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Fatal, category, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Fatal(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Fatal, LogCategory{}, fmt, std::forward<Args>(args)...);
}

inline const char* Logger::LevelToString(const LogLevel level)
{
	switch (level)
	{ // clang-format off
	case LogLevel::Info:    return "INFO";
	case LogLevel::Warning: return "WARN";
	case LogLevel::Error:   return "ERROR";
	case LogLevel::Fatal:   return "FATAL";
	default:                return "UNKNOWN";
	} // clang-format on
}

} // namespace re