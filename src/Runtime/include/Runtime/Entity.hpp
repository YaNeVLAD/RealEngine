#pragma once

#include <stdexcept>

#include <entt/entt.hpp>

namespace re::runtime
{

class Scene;

class Entity
{
public:
	Entity(entt::entity entity, entt::registry& registry);
	Entity(Entity const&) = default;

	template <typename TComponent, typename... TArgs>
	Entity Add(TArgs&&... args);

	template <typename... TComponents>
	[[nodiscard]] decltype(auto) Get();

	template <typename... TComponents>
	[[nodiscard]] decltype(auto) Get() const;

	template <typename... TComponents>
	[[nodiscard]] bool Has() const;

	template <typename TComponent>
	void Remove();

	void Invalidate();

	[[nodiscard]] bool IsValid() const;

	bool operator==(const Entity& other) const;
	bool operator!=(const Entity& other) const;

	explicit operator bool() const;
	explicit operator entt::entity() const;

private:
	void AssertValid() const;

private:
	entt::entity m_entity{ entt::null };
	entt::registry& m_registry;
};

} // namespace re::runtime

#include <Runtime/Entity.inl>