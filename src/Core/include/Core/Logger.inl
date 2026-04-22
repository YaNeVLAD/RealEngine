#include <Core/Logger.hpp>

namespace re
{

template <typename... Args>
constexpr void Logger::Log(const LogLevel level, std::format_string<Args...> fmt, Args&&... args)
{
	std::string levelStr = LevelToString(level);
	std::string message = std::format(fmt, std::forward<Args>(args)...);

	if (level == LogLevel::Error || level == LogLevel::Fatal)
	{
		std::cerr << std::format("[{}] {}\n", levelStr, message);
	}
	else
	{
		std::cout << std::format("[{}] {}\n", levelStr, message);
	}
}

template <typename... Args>
void Logger::Info(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Warn(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Error(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void Logger::Fatal(std::format_string<Args...> fmt, Args&&... args)
{
	Log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

inline const char* Logger::LevelToString(const LogLevel level)
{
	switch (level)
	{
		// clang-format off
	case LogLevel::Info:    return "INFO";
	case LogLevel::Warning: return "WARN";
	case LogLevel::Error:   return "ERROR";
	case LogLevel::Fatal:   return "FATAL";
		// clang-format on
	default:
		return "UNKNOWN";
	}
}

} // namespace re