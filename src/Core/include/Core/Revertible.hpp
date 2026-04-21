#pragma once

#include <concepts>

namespace re
{

template <std::copyable T>
class Revertible
{
public:
	constexpr explicit Revertible(T default_value)
		: m_default(std::move(default_value))
		, m_current(m_default)
	{
	}

	[[nodiscard]] constexpr const T& Get() const noexcept
	{
		return m_current;
	}

	[[nodiscard]] constexpr const T& GetDefault() const noexcept
	{
		return m_default;
	}

	[[nodiscard]] constexpr T& RawRef() noexcept
	{
		return m_current;
	}

	[[nodiscard]] constexpr bool Modified() const noexcept
	{
		return m_current != m_default;
	}

	constexpr void Set(T new_value) noexcept(std::is_nothrow_copy_assignable_v<T>)
	{
		m_current = std::move(new_value);
	}

	constexpr void Reset() noexcept(std::is_nothrow_copy_assignable_v<T>)
	{
		if (Modified())
		{
			m_current = m_default;
		}
	}

	[[nodiscard]] constexpr explicit operator const T&() const
	{
		return m_current;
	}

	[[nodiscard]] constexpr const T& operator*() const
	{
		return m_current;
	}

	[[nodiscard]] constexpr const T* operator->() const noexcept
	{
		return &m_current;
	}

private:
	const T m_default;
	T m_current;
};

} // namespace re