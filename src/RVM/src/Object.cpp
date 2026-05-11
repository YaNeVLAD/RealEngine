#include <RVM/Object/Object.hpp>

#include <RVM/VirtualMachine.hpp>

namespace re::rvm
{

std::size_t Object::RefCount() const noexcept
{
	return m_refCount;
}

void Object::IncRef() noexcept
{
	++m_refCount;
}

void Object::Release() noexcept
{
	m_refCount--;
	if (m_refCount == 0)
	{
		if (m_vm)
		{
			m_vm->EnqueueForDestruction(this);
		}
		else
		{
			delete this;
		}
	}
}

void Object::SetVM(VirtualMachine* vm) noexcept
{
	m_vm = vm;
}

} // namespace re::rvm