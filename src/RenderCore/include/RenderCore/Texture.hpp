#pragma once

#if defined(RE_USE_FILAMENT_RENDER)
#include <RenderCore/Filament/Texture.hpp>
#elif defined(RE_USE_GLFW_RENDER)
#include <RenderCore/GLFW/Texture.hpp>
#elif defined(RE_USE_SFML_RENDER)
#include <RenderCore/SFML/Texture.hpp>
#endif