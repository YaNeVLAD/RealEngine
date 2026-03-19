#pragma once

#include <ECS/Entity/Entity.hpp>

#include <entt/entt.hpp>

#include <memory>
#include <tuple>

namespace re::ecs
{
#ifdef RE_USE_ENTT

template <typename... Components>
class EnttViewAdapter
{
public:
	explicit EnttViewAdapter(entt::registry& registry)
		: m_view(registry.view<Components...>())
	{
	}

	struct Iterator
	{
		using BaseIterator = decltype(std::declval<entt::view<entt::get_t<Components...>>>().each().begin());
		BaseIterator it;

		auto operator*()
		{
			return std::apply([](entt::entity e, Components&... comps) {
				return std::tuple<Entity, Components&...>(
					Entity{ static_cast<uint32_t>(e) },
					comps...);
			},
				*it);
		}

		Iterator& operator++()
		{
			++it;
			return *this;
		}

		bool operator!=(const Iterator& other) const { return it != other.it; }
	};

	Iterator begin() { return { m_view.each().begin() }; }
	Iterator end() { return { m_view.each().end() }; }

private:
	entt::view<entt::get_t<Components...>> m_view;
};

#endif
} // namespace re::ecs