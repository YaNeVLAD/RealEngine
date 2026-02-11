#include <Runtime/Application.hpp>

namespace re
{

template <typename TScene>
ecs::Scene& Application::CreateScene()
{
	return AddSceneImpl(TypeOf<TScene>().Name());
}

template <typename TScene>
void Application::ChangeScene()
{
	ChangeSceneImpl(TypeOf<TScene>().Name());
}

} // namespace re