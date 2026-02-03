#pragma once

#include <Core/Config.hpp>

#if defined(RE_ECS_EXPORTS)

#define RE_ECS_API RE_API_EXPORT

#else

#define RE_ECS_API RE_API_IMPORT

#endif