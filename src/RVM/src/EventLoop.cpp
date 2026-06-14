#include <RVM/EventLoop.hpp>

namespace re::rvm
{

void EventLoop::Delay(CoroutinePtr const& coroutine, const std::uint64_t milliseconds)
{
	auto wakeUpTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
	m_sleeping.emplace_back(coroutine, wakeUpTime);
}

InterpreterResult EventLoop::Run(VirtualMachine& vm)
{
	auto status = InterpreterResult::Success;

	while (!m_sleeping.empty())
	{
		auto now = std::chrono::steady_clock::now();
		bool hasReadyMacrotasks = false;

		for (auto it = m_sleeping.begin(); it != m_sleeping.end();)
		{
			if (now >= it->wakeUpTime)
			{
				vm.EnqueueMacrotask(it->coroutine);
				it = m_sleeping.erase(it);
				hasReadyMacrotasks = true;
			}
			else
			{
				++it;
			}
		}

		if (hasReadyMacrotasks)
		{
			status = vm.Resume();
			if (status == InterpreterResult::RuntimeError)
			{
				return status;
			}
		}
	}

	return status;
}

} // namespace re::rvm