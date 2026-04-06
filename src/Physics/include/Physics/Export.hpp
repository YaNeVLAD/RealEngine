#pragma once

#include <Core/Config.hpp>

#if defined(RE_PHYSICS_EXPORTS)

#define RE_PHYSICS_API RE_API_EXPORT

#else

#define RE_PHYSICS_API RE_API_IMPORT

#endif