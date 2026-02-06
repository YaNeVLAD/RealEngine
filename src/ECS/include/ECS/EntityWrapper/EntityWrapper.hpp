#pragma once

#include <ECS/Entity/Entity.hpp>
#include <ECS/Entity/Signature.hpp>

namespace re::ecs
{

template <typename TScene>
class EntityWrapper final
{
public:
	EntityWrapper(TScene* scene, Entity id, Signature signature);

	[[nodiscard]] Entity GetEntity() const;

	[[nodiscard]] Signature GetSignature() const;

	template <typename TComponent>
	EntityWrapper Add(TComponent const& component);

	template <typename TComponent, typename... TArgs>
	EntityWrapper Add(TArgs&&... args);

	template <typename TComponent>
	TComponent& Get();

	template <typename TComponent>
	TComponent const& Get() const;

	template <typename TComponent>
	[[nodiscard]] bool Has() const;

	template <typename TComponent>
	void Remove();

	void Destroy();

	auto operator<=>(EntityWrapper const& other) const;

	bool operator==(EntityWrapper const& other) const;

	bool operator==(Entity const& entity) const;

private:
	TScene* m_scene;

	Entity m_id;
	Signature m_signature;
};

} // namespace re::ecs

#include <ECS/EntityWrapper/EntityWrapper.inl>