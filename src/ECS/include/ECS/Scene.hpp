#pragma once

#if defined(RE_USE_ENTT)
#include <ECS/EnttAdapter/Scene_EnTT.hpp>
#else
#include <ECS/Scene/Scene.hpp>
#endif

namespace re::detail
{

template <typename T>
struct DirtyTag
{
	bool dummy = true;
};

struct TransparentTag
{
	bool dummy = true;
};

struct OpaqueTag
{
	bool dummy = true;
};

} // namespace re::detail