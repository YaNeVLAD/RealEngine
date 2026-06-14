#pragma once

#include <RVM/Export.hpp>

#include <cstddef>

namespace re::rvm
{

class VirtualMachine;

class RE_RVM_API Object
{
public:
	virtual ~Object() noexcept = default;

	[[nodiscard]] std::size_t RefCount() const noexcept;

	void IncRef() noexcept;

	void Release() noexcept;

	void SetVM(VirtualMachine* vm) noexcept;

	[[nodiscard]] Object* GetNext() noexcept;

	[[nodiscard]] const Object* GetNext() const noexcept;

	[[nodiscard]] bool IsMarked() const noexcept;

	void SetNext(Object* next) noexcept;

	void SetMarked(bool isMarked) noexcept;

	virtual void Trace(VirtualMachine* /* vm */) {}

private:
	std::size_t m_refCount = 0;

	bool m_isDestroying = false;
	bool m_isMarked = false;

	Object* m_next{};
	VirtualMachine* m_vm{};

	friend class VirtualMachine;
};

} // namespace re::rvm