#pragma once

#include <ECS/ComponentManager/ComponentManager.hpp>
#include <ECS/View/IView.hpp>
#include <ECS/View/Iterator/ViewIterator.hpp>

#include <vector>

namespace re::ecs
{

template <typename... TComponents>
class View final : public IView
{
public:
	using Iterator = ViewIterator<false, TComponents...>;
	using ConstIterator = ViewIterator<true, TComponents...>;

	View(ComponentManager& manager, Signature signature);

	auto begin();
	auto end();

	auto begin() const;
	auto end() const;

	void AddEntity(Entity entity);

	void OnEntityDestroyed(Entity entity) override;

	void OnEntitySignatureChanged(Entity entity, Signature entitySignature) override;

private:
	ComponentManager& m_manager;
	Signature m_signature;
	std::vector<Entity> m_entities;
};

} // namespace re::ecs

#include <ECS/View/View.inl>