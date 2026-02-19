#include <Runtime/Application.hpp>

namespace re
{

template <std::derived_from<Layout> TLayout, typename... TArgs>
void Application::AddLayout(TArgs&&... args)
{
	const auto layoutsSize = m_layouts.size();

	const auto type = TypeOf<TLayout>();
	const auto hash = type.Hash();
	const auto name = type.Name();

	if (m_layouts.contains(hash))
	{
		return;
	}

	auto& layout = *(m_layouts[hash] = std::make_shared<TLayout>(*this, std::forward<TArgs>(args)...));
	SetupScene(layout);
	layout.OnCreate();

	if (layoutsSize == 0)
	{
		SwitchLayoutImpl(name);
	}
}

template <std::derived_from<Layout> TLayout>
void Application::SwitchLayout()
{
	const auto name = TypeOf<TLayout>().Name();
	SwitchLayoutImpl(name);
}

} // namespace re