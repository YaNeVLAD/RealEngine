#include <ECS/ViewManager/ViewManager.hpp>

namespace re::ecs
{

void ViewManager::OnEntityDestroyed(const Entity entity)
{
	for (const auto& view : m_views | std::views::values)
	{
		view->OnEntityDestroyed(entity);
	}
}

void ViewManager::OnEntitySignatureChanged(const Entity entity, const Signature signature)
{
	for (const auto& view : m_views | std::views::values)
	{
		view->OnEntitySignatureChanged(entity, signature);
	}
}

}