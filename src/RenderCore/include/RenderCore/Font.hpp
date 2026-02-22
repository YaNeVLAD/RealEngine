#pragma once

#if defined(RE_USE_GLFW_RENDER)
#include <RenderCore/GLFW/Font.hpp>
#elif defined(RE_USE_SFML_RENDER)
#include <RenderCore/SFML/Font.hpp>
#endif