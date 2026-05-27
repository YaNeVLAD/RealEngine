#pragma once

#include <algorithm>
#include <array>
#include <stdexcept>

namespace re
{

template <typename TKey, typename TValue, std::size_t Size>
class flat_map final
{
public:
	using key_type = TKey;
	using mapped_type = TValue;
	using value_type = std::pair<TKey, TValue>;

	using const_pointer = const value_type*;
	using const_reference = const value_type&;

	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using const_iterator = std::array<std::pair<TKey, TValue>, Size>::const_iterator;
	using iterator = const_iterator;

	constexpr flat_map(const std::pair<TKey, TValue> (&arr)[Size])
	{
		std::ranges::copy(arr, m_data.begin());
		std::ranges::sort(m_data, std::ranges::less{}, &std::pair<TKey, TValue>::first);

		if constexpr (Size > 1)
		{
			for (std::size_t i = 0; i < Size - 1; ++i)
			{
				if (m_data[i].first == m_data[i + 1].first)
				{
					throw std::invalid_argument("FlatMap: Duplicate keys are not allowed.");
				}
			}
		}
	}

	template <typename K>
		requires std::three_way_comparable_with<TKey, K>
	[[nodiscard]] constexpr TValue const& get(K const& key, TValue const& fallback) const
	{
		auto it = find(key);

		return it != m_data.end() ? it->second : fallback;
	}

	template <typename K>
		requires std::three_way_comparable_with<TKey, K>
	[[nodiscard]] constexpr const TValue* try_get(K const& key) const noexcept
	{
		auto it = find(key);

		return it != m_data.end() ? &it->second : nullptr;
	}

	template <typename K>
		requires std::three_way_comparable_with<TKey, K>
	[[nodiscard]] constexpr const TValue* operator[](K const& key) const noexcept
	{
		return try_get(key);
	}

	template <typename K>
	[[nodiscard]] constexpr bool contains(K const& key) const noexcept
	{
		return find(key) != m_data.end();
	}

	[[nodiscard]] constexpr auto begin() const noexcept
	{
		return m_data.begin();
	}

	[[nodiscard]] constexpr auto end() const noexcept
	{
		return m_data.end();
	}

	[[nodiscard]] constexpr const_iterator cbegin() const noexcept
	{
		return m_data.cbegin();
	}

	[[nodiscard]] constexpr const_iterator cend() const noexcept
	{
		return m_data.cend();
	}

	[[nodiscard]] constexpr size_type size() const noexcept
	{
		return Size;
	}

	[[nodiscard]] constexpr bool empty() const noexcept
	{
		return Size != 0;
	}

	[[nodiscard]] constexpr bool not_empty() const noexcept
	{
		return Size == 0;
	}

private:
	template <typename K>
	[[nodiscard]] constexpr auto find(K const& key) const noexcept
	{
		const auto it = std::ranges::lower_bound(m_data, key, std::less{}, &std::pair<TKey, TValue>::first);
		if (it == m_data.end())
		{
			return m_data.end();
		}

		return it->first == key ? it : m_data.end();
	}

private:
	std::array<std::pair<TKey, TValue>, Size> m_data;
};

template <typename TKey, typename TValue, std::size_t Size>
flat_map(const std::pair<TKey, TValue> (&)[Size]) -> flat_map<TKey, TValue, Size>;

template <typename TKey, typename TValue, std::size_t Size>
[[nodiscard]] constexpr auto make_flat_map(const std::pair<TKey, TValue> (&arr)[Size])
{
	return flat_map<TKey, TValue, Size>(arr);
}

} // namespace re