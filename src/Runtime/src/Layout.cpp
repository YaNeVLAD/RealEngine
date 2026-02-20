#include <Runtime/Layout.hpp>

namespace re
{

Layout::Layout(Application& app)
	: m_app(app)
{
}

ecs::Scene& Layout::GetScene()
{
	return m_scene;
}

Application& Layout::GetApplication() const
{
	return m_app;
}

} // namespace re