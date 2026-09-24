#include <Runtime/Application.hpp>

namespace re
{

template <std::derived_from<Layout> TLayout, typename... TArgs>
TLayout& Application::AddLayout(TArgs&&... args)
{
	const auto layoutsSize = m_layouts.size();

	const auto type = TypeOf<TLayout>();
	const auto hash = type.Hash();
	const auto name = type.Name();

	if (const auto it = m_layouts.find(hash); it != m_layouts.end())
	{
		return static_cast<TLayout&>(*it->second);
	}

	auto stored = std::make_shared<TLayout>(*this, std::forward<TArgs>(args)...);
	auto& layout = *stored;
	m_layouts[hash] = std::move(stored);
	SetupScene(layout);
	layout.OnCreate();

	if (layoutsSize == 0)
	{
		SwitchLayoutImpl(name);
	}

	return layout;
}

template <std::derived_from<Layout> TLayout>
void Application::SwitchLayout()
{
	const auto name = TypeOf<TLayout>().Name();
	SwitchLayoutImpl(name);
}

} // namespace re