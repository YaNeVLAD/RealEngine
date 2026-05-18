#pragma once

#include <algorithm>
#include <array>
#include <optional>

namespace re
{

template <typename TKey, typename TValue, std::size_t Size>
class FlatMap final
{
public:
	constexpr FlatMap(const std::pair<TKey, TValue> (&arr)[Size])
	{
		std::ranges::copy(arr, m_data.begin());
		std::ranges::sort(m_data, std::less{}, &std::pair<TKey, TValue>::first);
	}

	template <typename K>
	[[nodiscard]] constexpr TValue Get(K const& key, TValue const& fallback) const
	{
		auto it = Find(key);
		if (it != m_data.end())
		{
			return it->second;
		}

		return fallback;
	}

	template <typename K>
	[[nodiscard]] constexpr std::optional<TValue> Get(K const& key) const
	{
		auto it = Find(key);
		if (it != m_data.end())
		{
			return it->second;
		}

		return std::nullopt;
	}

	template <typename K>
	[[nodiscard]] constexpr std::optional<TValue> operator[](K const& key) const
	{
		return Get(key);
	}

	template <typename K>
	[[nodiscard]] constexpr bool Contains(K const& key) const noexcept
	{
		const auto it = Find(key);

		return it != m_data.end();
	}

	[[nodiscard]] constexpr auto begin() const noexcept
	{
		return m_data.begin();
	}

	[[nodiscard]] constexpr auto end() const noexcept
	{
		return m_data.end();
	}

private:
	template <typename K>
	[[nodiscard]] constexpr auto Find(K const& key) const noexcept
	{
		const auto it = std::ranges::lower_bound(m_data, key, std::less{}, &std::pair<TKey, TValue>::first);
		if (it == m_data.end())
		{
			return m_data.end();
		}

		return it->first == key ? it : m_data.end();
	}

	std::array<std::pair<TKey, TValue>, Size> m_data;
};

template <typename TKey, typename TValue, std::size_t Size>
FlatMap(const std::pair<TKey, TValue> (&)[Size]) -> FlatMap<TKey, TValue, Size>;

} // namespace re