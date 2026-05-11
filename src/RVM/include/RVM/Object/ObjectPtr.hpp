#pragma once

#include <RVM/Export.hpp>

#include <RVM/Object/Object.hpp>

#include <type_traits>
#include <utility>

namespace re::rvm
{

template <typename T>
class RE_RVM_API ObjectPtr
{
public:
	ObjectPtr() noexcept = default;

	ObjectPtr(T* ptr) noexcept
		: m_ptr(ptr)
	{
		if (m_ptr)
		{
			m_ptr->IncRef();
		}
	}

	ObjectPtr(const ObjectPtr& other) noexcept
		: m_ptr(other.m_ptr)
	{
		if (m_ptr)
		{
			m_ptr->IncRef();
		}
	}

	ObjectPtr& operator=(const ObjectPtr& other) noexcept
	{
		if (this != &other)
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}

			m_ptr = other.m_ptr;

			if (m_ptr)
			{
				m_ptr->IncRef();
			}
		}

		return *this;
	}

	ObjectPtr(ObjectPtr&& other) noexcept
		: m_ptr(std::exchange(other.m_ptr, nullptr))
	{
	}

	ObjectPtr& operator=(ObjectPtr&& other) noexcept
	{
		if (this != &other)
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}
			m_ptr = std::exchange(other.m_ptr, nullptr);
		}

		return *this;
	}

	~ObjectPtr() noexcept
	{
		if (m_ptr)
		{
			m_ptr->Release();
		}
	}

	T* Get() const noexcept
	{
		return m_ptr;
	}

	T* operator->() const noexcept
	{
		return m_ptr;
	}

	T& operator*() const noexcept
	{
		return *m_ptr;
	}

	explicit operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	bool operator==(const ObjectPtr& other) const noexcept
	{
		return m_ptr == other.m_ptr;
	}

	bool operator!=(const ObjectPtr& other) const noexcept
	{
		return m_ptr != other.m_ptr;
	}

private:
	T* m_ptr{};
};

template <typename T, typename... TArgs>
	requires std::constructible_from<T, TArgs...>
ObjectPtr<T> MakeObject(TArgs&&... args) noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
{
	return ObjectPtr<T>(new T(std::forward<TArgs>(args)...));
}

template <typename TVirtualMachine>
class ObjectAllocator
{
public:
	template <typename T, typename... TArgs>
		requires std::constructible_from<T, TArgs...>
	ObjectPtr<T> Allocate(TArgs&&... args) noexcept(std::is_nothrow_constructible_v<T, TArgs...>)
	{
		T* obj = new T(std::forward<TArgs>(args)...);

		auto* vm = static_cast<TVirtualMachine*>(this);
		obj->SetVM(vm);

		obj->SetNext(m_allObjects);
		m_allObjects = obj;

		vm->OnObjectAllocated();

		return ObjectPtr<T>(obj);
	}

	[[nodiscard]] Object** AllocatedObjects() noexcept
	{
		return &m_allObjects;
	}

private:
	Object* m_allObjects{};
};

} // namespace re::rvm