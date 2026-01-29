#pragma once

#include <Core/Types.hpp>

namespace re::runtime
{

class Scene;

class System
{
public:
	virtual ~System() = default;

	virtual void OnCreate(Scene& scene) {}

	virtual void OnUpdate(Scene& scene, core::TimeDelta dt) = 0;
};

} // namespace re::runtime