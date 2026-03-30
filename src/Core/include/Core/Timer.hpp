#pragma once

#include <chrono>
#include <format>
#include <iostream>

namespace re
{

class Timer
{
	using Clock = std::chrono::high_resolution_clock;

public:
	explicit Timer(const std::string_view name)
		: m_name(name)
	{
	}

	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;

	Timer(Timer&&) = delete;
	Timer& operator=(Timer&&) = delete;

	~Timer()
	{
		Stop();
	}

	double Stop()
	{
		if (!m_running)
		{
			return 0;
		}
		m_running = false;

		const auto curr = Clock::now();
		const auto dur = curr - m_start;
		std::cout << std::format("{} took {} ns ({} s)\n", m_name, dur.count(), ToSeconds(dur));

		return ToSeconds(dur);
	}

private:
	std::string_view m_name;
	bool m_running = true;

	Clock::time_point m_start = Clock::now();

	static double ToSeconds(const std::chrono::nanoseconds nanoseconds)
	{
		return std::chrono::duration<double>(nanoseconds).count();
	}
};

template <typename Fn, typename... Args>
	requires std::is_invocable_v<Fn, Args...>
double MeasureTime(const std::string_view name, Fn&& fn, Args&&... args)
{
	Timer timer(name);

	std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);

	return timer.Stop();
}

#define _RE_TIMER_CONCAT_IMPL(x, y) x##y
#define _RE_TIMER_CONCAT(x, y) _RE_TIMER_CONCAT_IMPL(x, y)

#define RE_PROFILE_SCOPE(name) re::Timer _RE_TIMER_CONCAT(timer_, __LINE__)(name)

#if defined(__GNUC__) || defined(__clang__)

#define RE_PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)

#elif defined(_MSC_VER)

#define RE_PROFILE_FUNCTION() RE_PROFILE_SCOPE(__FUNCSIG__)

#else

#define RE_PROFILE_FUNCTION() PROFILE_SCOPE(__func__)

#endif

} // namespace re