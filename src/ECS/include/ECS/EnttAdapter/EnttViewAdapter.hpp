#pragma once

#include <ECS/Entity/Entity.hpp>

#include <entt/entt.hpp>

#include <memory>
#include <tuple>

namespace re::ecs
{
#ifdef RE_USE_ENTT

template <typename... TComponents>
class EnttViewAdapter
{
public:
	explicit EnttViewAdapter(entt::registry& registry)
		: m_view(registry.view<TComponents...>())
	{
	}

	struct Iterator
	{
		using BaseIterator = decltype(std::declval<entt::view<entt::get_t<TComponents...>>>().each().begin());
		BaseIterator it;

		auto operator*()
		{
			return std::apply([]<typename... T>(entt::entity e, T&&... comps) {
				return std::tuple<Entity, T...>(
					Entity{ static_cast<uint32_t>(e) },
					std::forward<T>(comps)...);
			},
				*it);
		}

		Iterator& operator++()
		{
			++it;
			return *this;
		}

		bool operator!=(const Iterator& other) const
		{
			return it != other.it;
		}

		bool operator==(const Iterator& other) const = default;
	};

	Iterator begin()
	{
		return { m_view.each().begin() };
	}

	Iterator end()
	{
		return { m_view.each().end() };
	}

private:
	entt::view<entt::get_t<TComponents...>> m_view;
};

#endif
} // namespace re::ecs