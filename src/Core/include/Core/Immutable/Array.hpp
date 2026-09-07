#pragma once

#include <array>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>

#include <Core/Assert.hpp>

namespace re
{

namespace detail
{

constexpr std::size_t static_pow(const std::size_t base, const std::size_t exponent)
{
	std::size_t result = 1;
	for (std::size_t i = 0; i < exponent; ++i)
	{
		result *= base;
	}

	return result;
}

constexpr std::size_t calculate_height(const std::size_t size, const std::size_t branching)
{
	std::size_t height = 1;
	std::size_t capacity = branching;
	while (capacity < size)
	{
		capacity *= branching;
		height++;
	}

	return height;
}

} // namespace detail

template <typename T, std::size_t N, std::size_t B = 32>
class ImmutableArray
{
	static_assert(N > 0, "Array size must be greater than 0");
	static_assert(B >= 2, "Branching should be at least 2");

	static constexpr std::size_t HEIGHT = detail::calculate_height(N, B);

	template <std::size_t H>
	struct Node
	{
		using value_type = std::conditional_t<H == 0,
			T,
			std::shared_ptr<const Node<H - 1>>>;

		std::array<value_type, B> children{};
	};

	using RootNode = Node<HEIGHT - 1>;

public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = T const&;
	using const_reference = T const&;
	using pointer = const T*;
	using const_pointer = const T*;

	class const_iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = T const&;

		const_iterator() noexcept
			: m_array(nullptr)
			, m_index(0)
		{
		}

		const_iterator(const ImmutableArray* array, const std::size_t index)
			: m_array(array)
			, m_index(index)
		{
		}

		reference operator*() const
		{
			RE_ASSERT(m_array != nullptr, "Dereferencing end or null iterator");

			return (*m_array)[m_index];
		}

		pointer operator->() const
		{
			return &(**this);
		}

		const_iterator& operator++() noexcept
		{
			++m_index;

			return *this;
		}
		const_iterator operator++(int) noexcept
		{
			const_iterator tmp = *this;
			++(*this);

			return tmp;
		}

		const_iterator& operator--() noexcept
		{
			--m_index;

			return *this;
		}
		const_iterator operator--(int) noexcept
		{
			const_iterator tmp = *this;
			--(*this);

			return tmp;
		}

		const_iterator& operator+=(const difference_type offset) noexcept
		{
			m_index += offset;

			return *this;
		}
		const_iterator operator+(const difference_type offset) const noexcept
		{
			const_iterator tmp = *this;

			return tmp += offset;
		}

		friend const_iterator operator+(const difference_type offset, const const_iterator& it) noexcept
		{
			return it + offset;
		}

		const_iterator& operator-=(const difference_type offset) noexcept
		{
			m_index -= offset;

			return *this;
		}
		const_iterator operator-(const difference_type offset) const noexcept
		{
			const_iterator tmp = *this;

			return tmp -= offset;
		}

		difference_type operator-(const const_iterator& other) const noexcept
		{
			return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
		}

		reference operator[](const difference_type offset) const
		{
			return *(*this + offset);
		}

		auto operator<=>(const const_iterator& other) const noexcept
		{
			RE_ASSERT(m_array == other.m_array, "Comparing iterators from different containers");

			return m_index <=> other.m_index;
		}

		bool operator==(const const_iterator& other) const noexcept
		{
			return m_array == other.m_array && m_index == other.m_index;
		}

	private:
		const ImmutableArray* m_array;
		size_type m_index;
	};

	class Transient
	{
		friend class ImmutableArray;

	public:
		Transient& set(const std::size_t index, T value)
		{
			RE_ASSERT(index < N, "Out of bounds access");
			m_root = set_transient_impl<HEIGHT - 1>(m_root, index, std::move(value));
			return *this;
		}

		T const& get(const std::size_t index) const
		{
			RE_ASSERT(index < N, "Out of bounds access");
			return ImmutableArray::get_impl<HEIGHT - 1>(m_root.get(), index);
		}

		[[nodiscard]] ImmutableArray persistent() &&
		{
			return ImmutableArray(std::move(m_root));
		}

	private:
		std::shared_ptr<const RootNode> m_root;

		explicit Transient(std::shared_ptr<const RootNode> root)
			: m_root(std::move(root))
		{
		}

		template <std::size_t H>
		static std::shared_ptr<const Node<H>> set_transient_impl(
			std::shared_ptr<const Node<H>> const& current_node,
			const std::size_t index,
			T value)
		{
			constexpr std::size_t child_span = detail::static_pow(B, H);
			std::size_t child_idx = (index / child_span) % B;

			std::shared_ptr<Node<H>> mutable_node;

			if (current_node && current_node.use_count() == 1)
			{
				mutable_node = std::const_pointer_cast<Node<H>>(current_node);
			}
			else
			{
				mutable_node = current_node ? std::make_shared<Node<H>>(*current_node)
											: std::make_shared<Node<H>>();
			}

			if constexpr (H == 0)
			{
				mutable_node->children[child_idx] = std::move(value);
			}
			else
			{
				mutable_node->children[child_idx] = set_transient_impl<H - 1>(
					mutable_node->children[child_idx],
					index,
					std::move(value));
			}

			return mutable_node;
		}
	};

	using iterator = const_iterator;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using reverse_iterator = const_reverse_iterator;

public:
	ImmutableArray()
	{
		m_root = std::make_shared<RootNode>();
	}

	ImmutableArray(std::initializer_list<T> list)
	{
		auto tmp = Transient(std::make_shared<RootNode>());
		size_type index = 0;

		for (const auto& item : list)
		{
			if (index >= N)
			{
				break;
			}

			tmp.set(index++, item);
		}

		m_root = std::move(tmp).persistent().m_root;
	}

	ImmutableArray(ImmutableArray const&) = default;
	ImmutableArray& operator=(ImmutableArray const&) = default;

	ImmutableArray(ImmutableArray&&) noexcept = default;
	ImmutableArray& operator=(ImmutableArray&&) noexcept = default;

	~ImmutableArray() = default;

	[[nodiscard]] Transient transient() const&
	{
		return Transient(m_root);
	}

	[[nodiscard]] Transient transient() &&
	{
		return Transient(std::move(m_root));
	}

	const_reference get(const size_type index) const
	{
		RE_ASSERT(index < N, "Out of bounds access");

		return get_impl<HEIGHT - 1>(m_root.get(), index);
	}

	[[nodiscard]] const_reference operator[](const size_type index) const
	{
		RE_ASSERT(index < N, "Out of bounds access");

		return get_impl<HEIGHT - 1>(m_root.get(), index);
	}

	[[nodiscard]] const_reference at(const size_type index) const
	{
		if (index >= N)
		{
			throw std::out_of_range("re::ImmutableArray::at index out of range");
		}

		return get_impl<HEIGHT - 1>(m_root.get(), index);
	}

	[[nodiscard]] const_reference front() const
	{
		return (*this)[0];
	}

	[[nodiscard]] const_reference back() const
	{
		return (*this)[N - 1];
	}

	[[nodiscard]] ImmutableArray set(const size_type index, T value) const&
	{
		RE_ASSERT(index < N, "Out of bounds access");
		auto new_root = set_pure_immutable_impl<HEIGHT - 1>(m_root, index, std::move(value));

		return ImmutableArray(std::move(new_root));
	}

	[[nodiscard]] ImmutableArray set(const size_type index, T value) &&
	{
		RE_ASSERT(index < N, "Out of bounds access");
		auto t = std::move(*this).transient();
		t.set(index, std::move(value));

		return std::move(t).persistent();
	}

	[[nodiscard]] constexpr std::size_t size() const noexcept
	{
		return N;
	}

	[[nodiscard]] constexpr bool empty() const noexcept
	{
		return N == 0;
	}

	[[nodiscard]] constexpr std::size_t height() const noexcept
	{
		return HEIGHT;
	}

	[[nodiscard]] const_iterator begin() const noexcept
	{
		return const_iterator(this, 0);
	}

	[[nodiscard]] const_iterator end() const noexcept
	{
		return const_iterator(this, N);
	}

	[[nodiscard]] const_iterator cbegin() const noexcept
	{
		return begin();
	}

	[[nodiscard]] const_iterator cend() const noexcept
	{
		return end();
	}

	[[nodiscard]] const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}

	[[nodiscard]] const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

	[[nodiscard]] const_reverse_iterator crbegin() const noexcept
	{
		return rbegin();
	}

	[[nodiscard]] const_reverse_iterator crend() const noexcept
	{
		return rend();
	}

	friend bool operator==(const ImmutableArray& lhs, const ImmutableArray& rhs)
	{
		if (&lhs == &rhs || lhs.m_root == rhs.m_root)
		{
			return true;
		}

		return std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}

private:
	std::shared_ptr<const RootNode> m_root;

	explicit ImmutableArray(std::shared_ptr<const RootNode> root)
		: m_root(std::move(root))
	{
	}

	template <std::size_t H>
	static const T& get_impl(const Node<H>* node, const std::size_t index)
	{
		if (!node)
		{
			static const T default_value{};

			return default_value;
		}

		constexpr std::size_t child_span = detail::static_pow(B, H);
		std::size_t child_index = (index / child_span) % B;

		if constexpr (H == 0)
		{
			return node->children[child_index];
		}
		else
		{
			return get_impl(node->children[child_index].get(), index);
		}
	}

	template <std::size_t H>
	static std::shared_ptr<const Node<H>> set_pure_immutable_impl(
		std::shared_ptr<const Node<H>> const& current_node,
		const std::size_t index,
		T value)
	{
		constexpr std::size_t child_span = detail::static_pow(B, H);
		std::size_t child_idx = (index / child_span) % B;

		auto new_node = current_node ? std::make_shared<Node<H>>(*current_node)
									 : std::make_shared<Node<H>>();

		if constexpr (H == 0)
		{
			new_node->children[child_idx] = std::move(value);
		}
		else
		{
			new_node->children[child_idx] = set_pure_immutable_impl<H - 1>(
				new_node->children[child_idx],
				index,
				std::move(value));
		}

		return new_node;
	}
};

} // namespace re