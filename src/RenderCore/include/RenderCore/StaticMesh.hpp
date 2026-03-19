#pragma once

#pragma once

#if defined(RE_USE_GLFW_RENDER)
#include <RenderCore/GLFW/StaticMesh.hpp>
#elif defined(RE_USE_SFML_RENDER)
#include <RenderCore/SFML/StaticMesh.hpp>
#endif