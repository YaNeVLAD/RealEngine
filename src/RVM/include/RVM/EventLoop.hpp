#pragma once

#include <RVM/Export.hpp>

#include <RVM/VirtualMachine.hpp>

#include <chrono>
#include <vector>

namespace re::rvm
{

class RE_RVM_API EventLoop
{
	using TimePoint = std::chrono::steady_clock::time_point;

public:
	void Delay(CoroutinePtr const& coroutine, std::uint64_t milliseconds);

	InterpreterResult Run(VirtualMachine& vm);

private:
	struct TimerTask
	{
		CoroutinePtr coroutine;
		TimePoint wakeUpTime;
	};

	std::vector<TimerTask> m_sleeping;
};

} // namespace re::rvm