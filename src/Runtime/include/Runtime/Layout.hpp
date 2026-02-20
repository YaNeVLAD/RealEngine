#pragma once

#include <Runtime/Export.hpp>

#include <ECS/Scene/Scene.hpp>
#include <RenderCore/Event.hpp>

namespace re
{

class Application;

class RE_RUNTIME_API Layout
{
public:
	explicit Layout(Application& app);
	virtual ~Layout() = default;

	virtual void OnCreate() {}

	virtual void OnAttach() {}

	virtual void OnDetach() {}

	virtual void OnUpdate(core::TimeDelta) {}

	virtual void OnEvent([[maybe_unused]] Event const& event) {}

	[[nodiscard]] ecs::Scene& GetScene();

protected:
	Application& GetApplication() const;

private:
	Application& m_app;
	ecs::Scene m_scene;
};

} // namespace re