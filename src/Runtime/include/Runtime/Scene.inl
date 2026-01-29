#include <Runtime/Scene.hpp>

namespace re::runtime
{

template <typename TSystem, typename... TArgs>
TSystem& Scene::AddSystem(TArgs&&... args)
{
	auto system = std::make_unique<TSystem>(std::forward<TArgs>(args)...);

	system->OnCreate(*this);

	m_systems.emplace_back(std::move(system));

	return *static_cast<TSystem*>(system.get());
}

template <typename... TComponents>
decltype(auto) Scene::View()
{
	return m_registry.view<TComponents...>();
}

} // namespace re::runtime