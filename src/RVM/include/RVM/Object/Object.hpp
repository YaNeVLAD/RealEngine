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

private:
	std::size_t m_refCount{};

	bool m_isMarked{};
	Object* next{};

	VirtualMachine* m_vm{};

	friend class VirtualMachine;
};

} // namespace re::rvm