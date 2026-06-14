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
	if (m_isDestroying)
	{
		return;
	}

	m_refCount--;
	if (m_refCount == 0)
	{
		m_isDestroying = true;

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

Object* Object::GetNext() noexcept
{
	return m_next;
}

const Object* Object::GetNext() const noexcept
{
	return m_next;
}

bool Object::IsMarked() const noexcept
{
	return m_isMarked;
}

void Object::SetNext(Object* next) noexcept
{
	m_next = next;
}

void Object::SetMarked(const bool isMarked) noexcept
{
	m_isMarked = isMarked;
}

} // namespace re::rvm