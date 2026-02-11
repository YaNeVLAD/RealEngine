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

} // namespace re